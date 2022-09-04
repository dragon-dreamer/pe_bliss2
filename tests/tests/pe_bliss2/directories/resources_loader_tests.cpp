#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/detail/packed_serialization.h"
#include "pe_bliss2/detail/resources/image_resource_directory.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/resources/resource_directory.h"
#include "pe_bliss2/resources/resource_directory_loader.h"
#include "pe_bliss2/pe_types.h"

#include "tests/tests/pe_bliss2/image_helper.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::resources;

namespace
{
class ResourcesLoaderTestFixture : public ::testing::Test
{
public:
	ResourcesLoaderTestFixture()
		: instance(create_test_image({ .start_section_rva = section_rva,
			.sections = { { section_virtual_size, section_raw_size } } }))
	{
		instance.get_section_data_list()[0].data()
			->set_absolute_offset(absolute_offset);
	}

	void add_resource_dir()
	{
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::resource).get()
			= { .virtual_address = directory_rva, .size = directory_size };
	}

	void add_virtual_resource_dir()
	{
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::resource).get()
			= { .virtual_address = directory_rva, .size = physical_part_size };
		instance.get_section_data_list()[0].copied_data().resize(physical_part_size);
	}

	void add_resource_dir_to_headers()
	{
		instance.get_full_headers_buffer().copied_data().resize(0x1000);
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::resource).get()
			= { .virtual_address = 0x20u, .size = 0x20u };
	}

	template<typename Array>
	void add_data(std::uint32_t rva, const Array& arr)
	{
		auto* data = instance.get_section_data_list()[0].copied_data().data()
			+ (directory_rva - section_rva)
			+ (rva - section_rva);
		std::copy(arr.begin(), arr.end(), data);
	}

	void add_resource_dir_descriptors()
	{
		add_data(directory_rva, resource_dir0);
		add_data(directory_rva + dir0_entry0_dir1_offset, resource_dir1);
		add_data(directory_rva + dir0_entry1_data_offset,
			resource_data_descriptor);
		add_data(data_rva, resource_data);
		add_data(directory_rva + dir0_entry1_name_offset,
			dir0_entry1_name);
	}

public:
	void validate_resources(const std::optional<resource_directory_details>& dir0,
		bool copy_raw_data)
	{
		ASSERT_TRUE(dir0);
		expect_contains_errors(*dir0, resource_directory_loader_errc::unsorted_entries);

		const auto& dir0_entries = dir0->get_entries();
		ASSERT_EQ(dir0_entries.size(), number_of_named_entries_0 + number_of_id_entries_0);
		expect_contains_errors(dir0_entries[0]);
		const auto* dir0_id0 = std::get_if<resource_id_type>(
			&dir0_entries[0].get_name_or_id());
		ASSERT_NE(dir0_id0, nullptr);
		EXPECT_EQ(*dir0_id0, dir0_entry0_id);
		expect_contains_errors(dir0_entries[1]);
		const auto* dir0_name1 = std::get_if<packed_utf16_string>(
			&dir0_entries[1].get_name_or_id());
		ASSERT_NE(dir0_name1, nullptr);
		EXPECT_EQ(dir0_name1->value(), u"abc");
		EXPECT_EQ(dir0_name1->get_state().absolute_offset(),
			absolute_offset + dir0_entry1_name_offset);

		const auto* dir1 = std::get_if<resource_directory_details>(
			&dir0_entries[0].get_data_or_directory());
		expect_contains_errors(*dir1,
			resource_directory_loader_errc::invalid_number_of_named_and_id_entries);
		ASSERT_NE(dir1, nullptr);
		const auto& dir1_entries = dir1->get_entries();
		ASSERT_EQ(dir1_entries.size(), number_of_named_entries_1 + number_of_id_entries_1);
		expect_contains_errors(dir1_entries[0]);
		const auto* dir1_id0 = std::get_if<resource_id_type>(
			&dir1_entries[0].get_name_or_id());
		ASSERT_NE(dir1_id0, nullptr);
		EXPECT_EQ(*dir1_id0, dir1_entry0_id);
		const auto* dir1_loop0 = std::get_if<rva_type>(
			&dir1_entries[0].get_data_or_directory());
		ASSERT_NE(dir1_loop0, nullptr);
		EXPECT_EQ(*dir1_loop0, directory_rva);

		const auto* data1 = std::get_if<resource_data_entry_details>(
			&dir0_entries[1].get_data_or_directory());
		ASSERT_NE(data1, nullptr);
		EXPECT_EQ(data1->get_raw_data().data()->absolute_offset(),
			absolute_offset + (data_rva - section_rva));
		ASSERT_EQ(data1->get_raw_data().is_copied(), copy_raw_data);
		std::array<std::byte, resource_data.size()> data{};
		ASSERT_EQ(data1->get_raw_data().data()->read(0,
			resource_data.size(), data.data()), resource_data.size());
		EXPECT_EQ(data, resource_data);
	}

public:
	image::image instance;

public:
	static constexpr std::uint32_t absolute_offset = 0x1000u;
	static constexpr std::uint32_t section_rva = 0x1000u;
	static constexpr std::uint32_t directory_rva = section_rva;
	static constexpr std::uint32_t section_virtual_size = 0x2000u;
	static constexpr std::uint32_t section_raw_size = 0x1000u;

	static constexpr std::size_t physical_part_size
		= detail::packed_reflection::get_type_size<
			detail::resources::image_resource_directory>() - 1u;

	static constexpr std::uint8_t number_of_named_entries_0 = 1u;
	static constexpr std::uint8_t number_of_id_entries_0 = 1u;
	static constexpr std::uint8_t number_of_named_entries_1 = 1u;
	static constexpr std::uint8_t number_of_id_entries_1 = 0u;
	static constexpr std::uint32_t dir0_entry0_id = 0x1234567u;
	static constexpr std::uint32_t dir1_entry0_id = 0xabcdefu;
	static constexpr std::uint32_t dir0_entry0_dir1_offset = 0x100u;
	static constexpr std::uint32_t dir0_entry1_name_offset = 0x200u;
	static constexpr std::uint32_t dir0_entry1_data_offset = 0x300u;
	static constexpr std::uint32_t data_rva = section_rva + 0x400u;
	static constexpr std::uint32_t data_size = 10u;
	static constexpr std::array resource_dir0{
		//dir 0
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //characteristics
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //time_date_stamp
		std::byte{}, std::byte{}, //major_version
		std::byte{}, std::byte{}, //minor_version
		std::byte{number_of_named_entries_0}, std::byte{}, //number_of_named_entries
		std::byte{number_of_id_entries_0}, std::byte{}, //number_of_id_entries
		//dir 0 - entry 0
		//name_or_id
		std::byte{dir0_entry0_id & 0xffu},
		std::byte{(dir0_entry0_id >> 8u) & 0xffu},
		std::byte{(dir0_entry0_id >> 16u) & 0xffu},
		std::byte{(dir0_entry0_id >> 24u) & 0xffu},
		//offset_to_data_or_directory
		std::byte{dir0_entry0_dir1_offset & 0xffu},
		std::byte{(dir0_entry0_dir1_offset >> 8u) & 0xffu},
		std::byte{(dir0_entry0_dir1_offset >> 16u) & 0xffu},
		std::byte{(0x80u | (dir0_entry0_dir1_offset >> 24u)) & 0xffu},
		//dir 0 - entry 1
		//name_or_id
		std::byte{dir0_entry1_name_offset & 0xffu},
		std::byte{(dir0_entry1_name_offset >> 8u) & 0xffu},
		std::byte{(dir0_entry1_name_offset >> 16u) & 0xffu},
		std::byte{(0x80u | (dir0_entry1_name_offset >> 24u)) & 0xffu},
		//offset_to_data_or_directory
		std::byte{dir0_entry1_data_offset & 0xffu},
		std::byte{(dir0_entry1_data_offset >> 8u) & 0xffu},
		std::byte{(dir0_entry1_data_offset >> 16u) & 0xffu},
		std::byte{(dir0_entry1_data_offset >> 24u) & 0xffu},
	};

	static constexpr std::array resource_dir1{
		//dir 1
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //characteristics
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //time_date_stamp
		std::byte{}, std::byte{}, //major_version
		std::byte{}, std::byte{}, //minor_version
		std::byte{number_of_named_entries_1}, std::byte{}, //number_of_named_entries
		std::byte{number_of_id_entries_1}, std::byte{}, //number_of_id_entries
		//dir 1 - entry 0
		//name_or_id
		std::byte{dir1_entry0_id & 0xffu},
		std::byte{(dir1_entry0_id >> 8u) & 0xffu},
		std::byte{(dir1_entry0_id >> 16u) & 0xffu},
		std::byte{(dir1_entry0_id >> 24u) & 0xffu},
		//offset_to_data_or_directory - loop to resource_dir0
		std::byte{}, std::byte{}, std::byte{}, std::byte{0x80u},
	};

	static constexpr std::array dir0_entry1_name{
		std::byte{3}, std::byte{}, //size
		std::byte{'a'}, std::byte{},
		std::byte{'b'}, std::byte{},
		std::byte{'c'}, std::byte{},
	};

	static constexpr std::array resource_data_descriptor{
		//offset_to_data
		std::byte{data_rva & 0xffu},
		std::byte{(data_rva >> 8u) & 0xffu},
		std::byte{(data_rva >> 16u) & 0xffu},
		std::byte{(data_rva >> 24u) & 0xffu},
		std::byte{data_size}, std::byte{}, std::byte{}, std::byte{}, //size
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //code_page
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //reserved
	};

	static constexpr std::array resource_data{
		std::byte{0}, std::byte{1}, std::byte{2}, std::byte{3},
		std::byte{4}, std::byte{5}, std::byte{6}, std::byte{7},
		std::byte{8}, std::byte{9},
	};

	static constexpr std::uint32_t directory_size = data_rva + data_size;
};
} //namespace

TEST_F(ResourcesLoaderTestFixture, AbsentDirectory)
{
	EXPECT_FALSE(resources::load(instance));
}

TEST_F(ResourcesLoaderTestFixture, ValidDirectory)
{
	add_resource_dir();
	add_resource_dir_descriptors();
	validate_resources(resources::load(instance), false);
}

TEST_F(ResourcesLoaderTestFixture, ValidDirectoryCopyBuffers)
{
	add_resource_dir();
	add_resource_dir_descriptors();
	validate_resources(resources::load(instance,
		{ .copy_raw_data = true }), true);
}

TEST_F(ResourcesLoaderTestFixture, HeaderDirectoryErr)
{
	add_resource_dir_to_headers();
	auto dir = resources::load(instance, { .include_headers = false });
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		resource_directory_loader_errc::invalid_resource_directory);
	EXPECT_TRUE(dir->get_entries().empty());
}

TEST_F(ResourcesLoaderTestFixture, HeaderDirectory)
{
	add_resource_dir_to_headers();
	auto dir = resources::load(instance);
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir);
	EXPECT_TRUE(dir->get_entries().empty());
}

TEST_F(ResourcesLoaderTestFixture, VirtualDirError)
{
	add_virtual_resource_dir();
	auto dir = resources::load(instance);
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		resource_directory_loader_errc::invalid_resource_directory);
	EXPECT_TRUE(dir->get_entries().empty());
}

TEST_F(ResourcesLoaderTestFixture, VirtualDir)
{
	add_virtual_resource_dir();
	auto dir = resources::load(instance, { .allow_virtual_data = true });
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		resource_directory_loader_errc::invalid_directory_size);
	EXPECT_TRUE(dir->get_entries().empty());
	EXPECT_EQ(dir->get_descriptor().physical_size(), physical_part_size);
}
