#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/exports/export_directory_loader.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/pe_types.h"

#include "tests/pe_bliss2/image_helper.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;

namespace
{
class ExportLoaderTestFixture : public ::testing::Test
{
public:
	ExportLoaderTestFixture()
		: instance(create_test_image({ .start_section_rva = section_rva,
			.sections = { { section_virtual_size, section_raw_size } } }))
	{
		instance.get_section_data_list()[0].data()
			->set_absolute_offset(absolute_offset);
	}

	void add_export_dir()
	{
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::exports).get()
			= { .virtual_address = section_rva, .size = 0x500u };
	}

	void add_export_dir_to_headers()
	{
		instance.get_full_headers_buffer().copied_data().resize(0x1000);
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::exports).get()
			= { .virtual_address = 0x20u, .size = 0x20u };
	}

	void add_virtual_export_dir()
	{
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::exports).get()
			= { .virtual_address = section_rva + section_raw_size - 1u, .size = 0x500u };
	}

	void add_library_name()
	{
		auto& data = instance.get_section_data_list()[0].copied_data();
		std::copy(export_dir_part1.begin(), export_dir_part1.end(), data.begin());

		auto lib_name_ptr = reinterpret_cast<const std::byte*>(lib_name);
		std::copy(lib_name_ptr, lib_name_ptr + sizeof(lib_name),
			data.begin() + lib_name_offset);
	}

	template<typename Addresses>
	void add_exported_addresses(const Addresses& addresses)
	{
		auto& data = instance.get_section_data_list()[0].copied_data();
		std::copy(export_dir_addresses.begin(), export_dir_addresses.end(),
			data.begin() + export_dir_part1.size());

		auto addresses_ptr = reinterpret_cast<const std::byte*>(addresses.data());
		std::copy(addresses_ptr, addresses_ptr + addresses.size(),
			data.begin() + addresses_offset);
	}

	void add_exported_addresses()
	{
		add_exported_addresses(addresses);
	}

	void add_fwd_addresses()
	{
		add_exported_addresses(fwd_addresses);

		auto& data = instance.get_section_data_list()[0].copied_data();
		auto fwd_name0_ptr = reinterpret_cast<const std::byte*>(fwd_name0);
		std::copy(fwd_name0_ptr, fwd_name0_ptr + sizeof(fwd_name0),
			data.begin() + fwd_name0_offset);
	}

	void add_exported_name_table()
	{
		auto& data = instance.get_section_data_list()[0].copied_data();
		std::copy(export_dir_names.begin(), export_dir_names.end(),
			data.begin() + export_dir_part1.size() + export_dir_addresses.size());
	}

	void add_exported_names()
	{
		auto& data = instance.get_section_data_list()[0].copied_data();
		
		auto names_ptr = reinterpret_cast<const std::byte*>(names.data());
		std::copy(names_ptr, names_ptr + names.size(),
			data.begin() + names_offset);

		auto name_ordinals_ptr = reinterpret_cast<const std::byte*>(name_ordinals.data());
		std::copy(name_ordinals_ptr, name_ordinals_ptr + name_ordinals.size(),
			data.begin() + name_ordinals_offset);
	}

	void add_name_strings()
	{
		auto& data = instance.get_section_data_list()[0].copied_data();

		auto names_ptr = reinterpret_cast<const std::byte*>(name1);
		std::copy(names_ptr, names_ptr + sizeof(name1),
			data.begin() + name1_offset);

		names_ptr = reinterpret_cast<const std::byte*>(name2);
		std::copy(names_ptr, names_ptr + sizeof(name2),
			data.begin() + name2_offset);
	}

	static void validate_library_name(const exports::export_directory_details& dir)
	{
		EXPECT_EQ(dir.get_library_name().value(), lib_name);
		EXPECT_EQ(dir.get_library_name().get_state().relative_offset(),
			lib_name_offset);
		EXPECT_EQ(dir.get_library_name().get_state().absolute_offset(),
			lib_name_offset + absolute_offset);
	}

public:
	image::image instance;

public:
	static constexpr const char lib_name[] = "library";
	static constexpr std::uint32_t absolute_offset = 0x1000u;
	static constexpr std::uint32_t section_rva = 0x1000u;
	static constexpr std::uint8_t export_base = 45u;
	static constexpr std::uint8_t number_of_functions = 3u;
	static constexpr std::uint8_t number_of_names = 4u;
	static constexpr std::uint32_t lib_name_offset = 0x400u;
	static constexpr std::uint32_t addresses_offset = 0x100u;
	static constexpr std::uint32_t names_offset = 0x200u;
	static constexpr std::uint32_t name_ordinals_offset = 0x300u;
	static constexpr std::uint32_t first_rva = 0x53412u;
	static constexpr std::uint32_t third_rva = 0x800u + section_rva;
	static constexpr std::uint32_t name1_offset = 0x900u;
	static constexpr std::uint32_t name2_offset = 0x906u;
	static constexpr std::uint32_t invalid_name0_rva = 0x53412u;
	static constexpr std::uint32_t fwd_name0_offset = 0x200u;
	static constexpr std::uint32_t fwd_name0_rva = fwd_name0_offset + section_rva;
	static constexpr std::uint32_t fwd_name1_rva = 0x300u + section_rva;
	static constexpr const char name1[] = "name2";
	static constexpr const char name2[] = "name1";
	static constexpr const char fwd_name0[] = "fwd_name0";
	static constexpr std::uint32_t section_virtual_size = 0x2000u;
	static constexpr std::uint32_t section_raw_size = 0x1000u;

	static constexpr std::array export_dir_part1{
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //characteristics
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //time_date_stamp
		std::byte{}, std::byte{}, //major_version
		std::byte{}, std::byte{}, //minor_version
		std::byte{0x00}, std::byte{0x14}, std::byte{}, std::byte{}, //name
		std::byte{export_base}, std::byte{}, std::byte{}, std::byte{}, //base
		std::byte{number_of_functions}, std::byte{}, std::byte{}, std::byte{}, //number_of_functions
		std::byte{number_of_names}, std::byte{}, std::byte{}, std::byte{}, //number_of_names
	};

	static constexpr std::array export_dir_addresses{
		std::byte{0x00}, std::byte{0x11u}, std::byte{}, std::byte{}, //address_of_functions
	};

	static constexpr std::array export_dir_names{
		std::byte{0x00}, std::byte{0x12u}, std::byte{}, std::byte{}, //address_of_names
		std::byte{0x00}, std::byte{0x13u}, std::byte{}, std::byte{}, //address_of_name_ordinals
	};

	static constexpr std::array addresses{
		std::byte{0x12u}, std::byte{0x34u}, std::byte{0x05u}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{0x00u}, std::byte{0x18u}, std::byte{}, std::byte{}
	};

	// Inside of export directory
	static constexpr std::array fwd_addresses{
		std::byte{0x00u}, std::byte{0x12u}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		std::byte{0x00u}, std::byte{0x13u}, std::byte{}, std::byte{}
	};

	static constexpr std::array names{
		std::byte{0x12u}, std::byte{0x34u}, std::byte{0x05u}, std::byte{},
		std::byte{0x00}, std::byte{0x19}, std::byte{}, std::byte{},
		std::byte{0x06}, std::byte{0x19}, std::byte{}, std::byte{},
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
	};

	static constexpr std::array name_ordinals{
		std::byte{0x01}, std::byte{},
		std::byte{0x02}, std::byte{},
		std::byte{0x02}, std::byte{},
		std::byte{0x00}, std::byte{},
	};
};
} //namespace

TEST_F(ExportLoaderTestFixture, GetExportsAbsentDirectory)
{
	EXPECT_FALSE(exports::load(instance));
}

TEST_F(ExportLoaderTestFixture, GetExportsZeroedDirectory)
{
	add_export_dir();
	auto dir = exports::load(instance);
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		exports::export_directory_loader_errc::invalid_library_name);
	EXPECT_TRUE(dir->get_export_list().empty());
}

TEST_F(ExportLoaderTestFixture, GetExportsDirectoryLibraryName)
{
	add_export_dir();
	add_library_name();
	auto dir = exports::load(instance);
	ASSERT_TRUE(dir);
	validate_library_name(*dir);
	expect_contains_errors(*dir,
		exports::export_directory_loader_errc::invalid_address_list,
		exports::export_directory_loader_errc::invalid_name_list);
}

TEST_F(ExportLoaderTestFixture, GetExportsDirectoryAddresses)
{
	add_export_dir();
	add_library_name();
	add_exported_addresses();
	auto dir = exports::load(instance);
	ASSERT_TRUE(dir);
	validate_library_name(*dir);
	expect_contains_errors(*dir,
		exports::export_directory_loader_errc::invalid_name_list);

	const auto& export_list = dir->get_export_list();
	// One export is zero
	ASSERT_EQ(export_list.size(),
		addresses.size() / sizeof(std::uint32_t) - 1u);
	EXPECT_TRUE(export_list[0].get_names().empty());
	EXPECT_EQ(export_list[0].get_rva_ordinal(), 0u);
	EXPECT_FALSE(export_list[0].get_forwarded_name().has_value());
	EXPECT_EQ(export_list[0].get_rva().get(), first_rva);
	EXPECT_EQ(export_list[0].get_rva().get_state().absolute_offset(),
		absolute_offset + addresses_offset);
	EXPECT_EQ(export_list[0].get_rva().get_state().relative_offset(),
		addresses_offset);

	EXPECT_TRUE(export_list[1].get_names().empty());
	EXPECT_EQ(export_list[1].get_rva_ordinal(), 2u);
	EXPECT_FALSE(export_list[1].get_forwarded_name().has_value());
	EXPECT_EQ(export_list[1].get_rva().get(), third_rva);
	EXPECT_EQ(export_list[1].get_rva().get_state().absolute_offset(),
		absolute_offset + addresses_offset + sizeof(rva_type) * 2u);
	EXPECT_EQ(export_list[1].get_rva().get_state().relative_offset(),
		addresses_offset + sizeof(rva_type) * 2u);
}

TEST_F(ExportLoaderTestFixture, GetExportsDirectoryNamesEmpty)
{
	add_export_dir();
	add_library_name();
	add_exported_addresses();
	add_exported_name_table();
	auto dir = exports::load(instance);
	ASSERT_TRUE(dir);
	validate_library_name(*dir);
	expect_contains_errors(*dir);
	ASSERT_EQ(dir->get_export_list().size(), 2u);
	expect_contains_errors(dir->get_export_list()[0],
		exports::export_directory_loader_errc::invalid_name_rva,
		exports::export_directory_loader_errc::invalid_rva);
	expect_contains_errors(dir->get_export_list()[1]);
	
	const auto& names = dir->get_export_list()[0].get_names();
	ASSERT_EQ(names.size(), number_of_names);
	for (std::uint32_t i = 0; i != number_of_names; ++i)
	{
		EXPECT_FALSE(names[i].get_name().has_value());
		EXPECT_EQ(names[i].get_name_ordinal().get(), 0u);
		EXPECT_EQ(names[i].get_name_rva().get(), 0u);

		EXPECT_EQ(names[i].get_name_ordinal().get_state().absolute_offset(),
			absolute_offset + name_ordinals_offset + i * sizeof(exports::ordinal_type));
		EXPECT_EQ(names[i].get_name_ordinal().get_state().relative_offset(),
			name_ordinals_offset + i * sizeof(exports::ordinal_type));

		EXPECT_EQ(names[i].get_name_rva().get_state().absolute_offset(),
			absolute_offset + names_offset + i * sizeof(rva_type));
		EXPECT_EQ(names[i].get_name_rva().get_state().relative_offset(),
			names_offset + i * sizeof(rva_type));
	}

	EXPECT_TRUE(dir->get_export_list()[1].get_names().empty());
}

namespace
{
void validate_exported_strings(const exports::export_directory_details& dir,
	std::string_view name1, std::string_view name2, bool validate_names0 = true,
	std::uint32_t number_of_names1 = 2u)
{
	if (validate_names0)
	{
		const auto& names0 = dir.get_export_list()[0].get_names();
		ASSERT_EQ(names0.size(), 1u);
		EXPECT_FALSE(names0[0].get_name().has_value());
		EXPECT_EQ(names0[0].get_name_ordinal().get(), 0u);
		EXPECT_EQ(names0[0].get_name_rva().get(), 0u);
	}

	const auto& names1 = dir.get_export_list()[1].get_names();
	ASSERT_EQ(names1.size(), number_of_names1);
	if (number_of_names1 > 0)
	{
		ASSERT_TRUE(names1[0].get_name().has_value());
		EXPECT_EQ(names1[0].get_name().value().value(), name1);
		EXPECT_EQ(names1[0].get_name_rva().get(),
			ExportLoaderTestFixture::name1_offset + ExportLoaderTestFixture::section_rva);
		EXPECT_EQ(names1[0].get_name().value().get_state().absolute_offset(),
			ExportLoaderTestFixture::absolute_offset
			+ ExportLoaderTestFixture::name1_offset);
		EXPECT_EQ(names1[0].get_name().value().get_state().relative_offset(),
			ExportLoaderTestFixture::name1_offset);
	}

	if (number_of_names1 > 1)
	{
		ASSERT_TRUE(names1[1].get_name().has_value());
		EXPECT_EQ(names1[1].get_name().value().value(), name2);
		EXPECT_EQ(names1[1].get_name_rva().get(),
			ExportLoaderTestFixture::name2_offset + ExportLoaderTestFixture::section_rva);
		EXPECT_EQ(names1[1].get_name().value().get_state().absolute_offset(),
			ExportLoaderTestFixture::absolute_offset
			+ ExportLoaderTestFixture::name2_offset);
		EXPECT_EQ(names1[1].get_name().value().get_state().relative_offset(),
			ExportLoaderTestFixture::name2_offset);
	}
}
} //namespace

TEST_F(ExportLoaderTestFixture, GetExportsDirectoryNames)
{
	add_export_dir();
	add_library_name();
	add_exported_addresses();
	add_exported_name_table();
	add_exported_names();
	auto dir = exports::load(instance);
	ASSERT_TRUE(dir);
	validate_library_name(*dir);
	expect_contains_errors(*dir,
		exports::export_directory_loader_errc::invalid_name_ordinal);
	ASSERT_EQ(dir->get_export_list().size(), 2u);
	expect_contains_errors(dir->get_export_list()[0],
		exports::export_directory_loader_errc::invalid_rva,
		exports::export_directory_loader_errc::invalid_name_rva);
	expect_contains_errors(dir->get_export_list()[1],
		exports::export_directory_loader_errc::empty_name);

	validate_exported_strings(*dir, "", "");
}

TEST_F(ExportLoaderTestFixture, GetExportsDirectoryNamesWithStrings)
{
	add_export_dir();
	add_library_name();
	add_exported_addresses();
	add_exported_name_table();
	add_exported_names();
	add_name_strings();
	auto dir = exports::load(instance);
	ASSERT_TRUE(dir);
	validate_library_name(*dir);
	expect_contains_errors(*dir,
		exports::export_directory_loader_errc::invalid_name_ordinal,
		exports::export_directory_loader_errc::unsorted_names);
	ASSERT_EQ(dir->get_export_list().size(), 2u);
	expect_contains_errors(dir->get_export_list()[0],
		exports::export_directory_loader_errc::invalid_rva,
		exports::export_directory_loader_errc::invalid_name_rva);
	expect_contains_errors(dir->get_export_list()[1]);

	validate_exported_strings(*dir, name1, name2);
}

TEST_F(ExportLoaderTestFixture, GetExportsDirectoryAddressesForwarded)
{
	add_export_dir();
	add_library_name();
	add_fwd_addresses();
	auto dir = exports::load(instance);
	ASSERT_TRUE(dir);
	validate_library_name(*dir);
	expect_contains_errors(*dir,
		exports::export_directory_loader_errc::invalid_name_list);

	const auto& export_list = dir->get_export_list();
	// One export is zero
	ASSERT_EQ(export_list.size(),
		addresses.size() / sizeof(std::uint32_t) - 1u);
	EXPECT_TRUE(export_list[0].get_names().empty());
	EXPECT_EQ(export_list[0].get_rva_ordinal(), 0u);
	ASSERT_TRUE(export_list[0].get_forwarded_name().has_value());
	EXPECT_EQ(export_list[0].get_rva().get(), fwd_name0_rva);
	EXPECT_EQ(export_list[0].get_forwarded_name().value().value(), fwd_name0);
	EXPECT_EQ(export_list[0].get_forwarded_name().value()
		.get_state().absolute_offset(), absolute_offset + fwd_name0_offset);
	EXPECT_EQ(export_list[0].get_forwarded_name().value()
		.get_state().relative_offset(), fwd_name0_offset);

	EXPECT_TRUE(export_list[1].get_names().empty());
	EXPECT_EQ(export_list[1].get_rva_ordinal(), 2u);
	ASSERT_TRUE(export_list[1].get_forwarded_name().has_value());
	EXPECT_EQ(export_list[1].get_rva().get(), fwd_name1_rva);
	EXPECT_EQ(export_list[1].get_forwarded_name().value().value(), "");
}

TEST_F(ExportLoaderTestFixture, GetExportsZeroedDirectoryHeadersError)
{
	add_export_dir_to_headers();
	auto dir = exports::load(instance, { .include_headers = false });
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		exports::export_directory_loader_errc::invalid_directory);
	EXPECT_TRUE(dir->get_export_list().empty());
}

TEST_F(ExportLoaderTestFixture, GetExportsZeroedDirectoryIncludeHeaders)
{
	add_export_dir_to_headers();
	auto dir = exports::load(instance, { .include_headers = true });
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir);
	EXPECT_TRUE(dir->get_export_list().empty());
}

TEST_F(ExportLoaderTestFixture, GetExportsDirectoryAddressesLimited)
{
	add_export_dir();
	add_library_name();
	add_exported_addresses();
	auto dir = exports::load(instance, { .max_number_of_functions = 1u });
	ASSERT_TRUE(dir);
	validate_library_name(*dir);
	expect_contains_errors(*dir,
		exports::export_directory_loader_errc::invalid_name_list,
		exports::export_directory_loader_errc::invalid_address_list_number_of_functions);

	const auto& export_list = dir->get_export_list();
	ASSERT_EQ(export_list.size(), 1u);
	EXPECT_TRUE(export_list[0].get_names().empty());
	EXPECT_EQ(export_list[0].get_rva_ordinal(), 0u);
	EXPECT_FALSE(export_list[0].get_forwarded_name().has_value());
	EXPECT_EQ(export_list[0].get_rva().get(), first_rva);
	EXPECT_EQ(export_list[0].get_rva().get_state().absolute_offset(),
		absolute_offset + addresses_offset);
	EXPECT_EQ(export_list[0].get_rva().get_state().relative_offset(),
		addresses_offset);
}

TEST_F(ExportLoaderTestFixture, GetExportsDirectoryNamesLimited)
{
	add_export_dir();
	add_library_name();
	add_exported_addresses();
	add_exported_name_table();
	add_exported_names();
	auto dir = exports::load(instance, { .max_number_of_names = 2u });
	ASSERT_TRUE(dir);
	validate_library_name(*dir);
	expect_contains_errors(*dir,
		exports::export_directory_loader_errc::invalid_name_ordinal,
		exports::export_directory_loader_errc::invalid_address_list_number_of_names);
	ASSERT_EQ(dir->get_export_list().size(), 2u);
	expect_contains_errors(dir->get_export_list()[0],
		exports::export_directory_loader_errc::invalid_rva);
	expect_contains_errors(dir->get_export_list()[1],
		exports::export_directory_loader_errc::empty_name);

	validate_exported_strings(*dir, "", "", false, 1u);
}

TEST_F(ExportLoaderTestFixture, GetExportsZeroedVitualDirectoryError)
{
	add_virtual_export_dir();
	auto dir = exports::load(instance);
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		exports::export_directory_loader_errc::invalid_directory);
	EXPECT_TRUE(dir->get_export_list().empty());
}

TEST_F(ExportLoaderTestFixture, GetExportsZeroedVitualDirectory)
{
	add_virtual_export_dir();
	auto dir = exports::load(instance, { .allow_virtual_data = true });
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		exports::export_directory_loader_errc::invalid_library_name);
	EXPECT_TRUE(dir->get_export_list().empty());
}
