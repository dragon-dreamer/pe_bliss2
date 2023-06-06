#include "gtest/gtest.h"

#include <array>
#include <cstdint>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/bound_import/bound_import_directory_loader.h"
#include "pe_bliss2/bound_import/bound_library.h"
#include "pe_bliss2/image/image.h"

#include "tests/pe_bliss2/image_helper.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;

namespace
{
class BoundImportLoaderTestFixture : public ::testing::Test
{
public:
	BoundImportLoaderTestFixture()
		: instance(create_test_image({ .start_section_rva = section_rva,
			.sections = { { section_virtual_size, section_raw_size } } }))
	{
		instance.get_section_data_list()[0].data()
			->set_absolute_offset(absolute_offset);
	}

	void add_bound_import_dir()
	{
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::bound_import).get()
			= { .virtual_address = section_rva, .size = 0x500u };
	}

	void add_virtual_bound_import_dir()
	{
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::bound_import).get()
			= { .virtual_address = section_rva + section_raw_size - 5u,
			.size = 0x500u };
	}

	void add_bound_import_dir_to_headers()
	{
		instance.get_full_headers_buffer().copied_data().resize(0x1000);
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::bound_import).get()
			= { .virtual_address = 0x20u, .size = 0x20u };
	}

	void add_bound_import_descriptor_with_names()
	{
		auto& data = instance.get_section_data_list()[0].copied_data();
		std::copy(bound_import_dir.begin(), bound_import_dir.end(), data.begin());

		auto name = reinterpret_cast<const std::byte*>(module1_name);
		std::copy(name, name + sizeof(module1_name),
			data.begin() + module1_name_offset);
		name = reinterpret_cast<const std::byte*>(module1_fwd1_name);
		std::copy(name, name + sizeof(module1_fwd1_name_offset),
			data.begin() + module1_fwd1_name_offset);
		name = reinterpret_cast<const std::byte*>(module1_fwd2_name);
		std::copy(name, name + sizeof(module1_fwd2_name),
			data.begin() + module1_fwd2_name_offset);
	}

public:
	image::image instance;

public:
	static constexpr std::uint32_t absolute_offset = 0x1000u;
	static constexpr std::uint32_t section_rva = 0x1000u;
	static constexpr std::uint32_t section_virtual_size = 0x1000u;
	static constexpr std::uint32_t section_raw_size = 0x800u;
	static constexpr std::uint32_t module1_name_offset = 0x500;
	static constexpr std::uint32_t module1_fwd1_name_offset = 0x510;
	static constexpr std::uint32_t module1_fwd2_name_offset = 0x520;
	static constexpr std::uint32_t module1_forwarders = 2u;
	static constexpr std::uint32_t module2_forwarders = 0u;
	static constexpr const char module1_name[] = "module1";
	static constexpr const char module1_fwd1_name[] = "fwd1";
	static constexpr const char module1_fwd2_name[] = "fwd2";

	static constexpr std::array bound_import_dir{
		// module 1
		std::byte{4}, std::byte{3}, std::byte{2}, std::byte{1}, //time_date_stamp
		std::byte{0x00}, std::byte{0x05}, //offset_module_name
		std::byte{module1_forwarders}, std::byte{}, //number_of_module_forwarder_refs

		// forwarder 1
		std::byte{8}, std::byte{7}, std::byte{6}, std::byte{5}, //time_date_stamp
		std::byte{0x10}, std::byte{0x05}, //offset_module_name
		std::byte{0xff}, std::byte{0xff}, //reserved

		// forwarder 2
		std::byte{0xc}, std::byte{0xb}, std::byte{0xa}, std::byte{9}, //time_date_stamp
		std::byte{0x20}, std::byte{0x05}, //offset_module_name
		std::byte{}, std::byte{}, //reserved

		// module 2
		std::byte{0xf}, std::byte{0xe}, std::byte{0xd}, std::byte{0}, //time_date_stamp
		std::byte{0x1}, std::byte{}, //offset_module_name (invalid)
		std::byte{module2_forwarders}, std::byte{}, //number_of_module_forwarder_refs
	};
};
} //namespace

TEST_F(BoundImportLoaderTestFixture, LoadAbsentBoundImportDirectory)
{
	EXPECT_FALSE(bound_import::load(instance));
}

TEST_F(BoundImportLoaderTestFixture, LoadZeroBoundImportDirectory)
{
	add_bound_import_dir();
	auto result = bound_import::load(instance);
	ASSERT_TRUE(result);
	EXPECT_TRUE(result->empty());
}

TEST_F(BoundImportLoaderTestFixture, LoadBoundImportDirectory)
{
	add_bound_import_dir();
	add_bound_import_descriptor_with_names();
	auto result = bound_import::load(instance);
	ASSERT_TRUE(result);
	const auto& entries = *result;
	ASSERT_EQ(entries.size(), 2u);

	expect_contains_errors(entries[0]);
	expect_contains_errors(entries[1],
		bound_import::bound_import_directory_loader_errc::name_offset_overlaps_descriptors);

	static constexpr std::uint32_t module_entry_size = 8u;
	EXPECT_EQ(entries[0].get_descriptor().get_state().absolute_offset(),
		absolute_offset);
	EXPECT_EQ(entries[1].get_descriptor().get_state().absolute_offset(),
		absolute_offset + (module1_forwarders + 1u) * module_entry_size);
	EXPECT_EQ(entries[0].get_descriptor().get_state().relative_offset(), 0u);
	EXPECT_EQ(entries[1].get_descriptor().get_state().relative_offset(),
		(module1_forwarders + 1u) * module_entry_size);

	EXPECT_EQ(entries[0].get_library_name().value(), module1_name);

	const auto& references1 = entries[0].get_references();
	ASSERT_EQ(references1.size(), module1_forwarders);
	expect_contains_errors(references1[0]);
	expect_contains_errors(references1[1]);
	EXPECT_EQ(references1[0].get_library_name().value(), module1_fwd1_name);
	EXPECT_EQ(references1[1].get_library_name().value(), module1_fwd2_name);
	EXPECT_EQ(references1[0].get_descriptor().get_state().absolute_offset(),
		absolute_offset + module_entry_size);
	EXPECT_EQ(references1[1].get_descriptor().get_state().absolute_offset(),
		absolute_offset + 2u * module_entry_size);
	EXPECT_EQ(references1[0].get_descriptor().get_state().relative_offset(),
		module_entry_size);
	EXPECT_EQ(references1[1].get_descriptor().get_state().relative_offset(),
		2u * module_entry_size);

	EXPECT_EQ(entries[1].get_references().size(), module2_forwarders);
}

TEST_F(BoundImportLoaderTestFixture, LoadVirtualBoundImportDirectoryError)
{
	add_virtual_bound_import_dir();
	auto result = bound_import::load(instance);
	ASSERT_TRUE(result);
	ASSERT_EQ(result->size(), 1u);
	expect_contains_errors(result->at(0),
		bound_import::bound_import_directory_loader_errc::invalid_bound_import_entry);
}

TEST_F(BoundImportLoaderTestFixture, LoadVirtualBoundImportDirectory)
{
	add_virtual_bound_import_dir();
	auto result = bound_import::load(instance, { .allow_virtual_data = true });
	ASSERT_TRUE(result);
	EXPECT_TRUE(result->empty());
}

TEST_F(BoundImportLoaderTestFixture, LoadHeadersBoundImportDirectoryError)
{
	add_bound_import_dir_to_headers();
	auto result = bound_import::load(instance, { .include_headers = false });
	ASSERT_TRUE(result);
	ASSERT_EQ(result->size(), 1u);
	expect_contains_errors(result->at(0),
		bound_import::bound_import_directory_loader_errc::invalid_bound_import_entry);
}

TEST_F(BoundImportLoaderTestFixture, LoadHeadersBoundImportDirectory)
{
	add_bound_import_dir_to_headers();
	auto result = bound_import::load(instance);
	ASSERT_TRUE(result);
	EXPECT_TRUE(result->empty());
}
