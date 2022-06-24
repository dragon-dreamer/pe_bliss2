#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <limits>

#include "buffers/input_container_buffer.h"

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/core/image_signature.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/core/optional_header.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_errc.h"
#include "pe_bliss2/section/section_header.h"

#include "tests/tests/pe_bliss2/image_helper.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

#include "utilities/generic_error.h"

using namespace pe_bliss;
using namespace pe_bliss::image;
using pe_image = pe_bliss::image::image;

TEST(ImageTests, EmptyTest)
{
	pe_image instance;
	EXPECT_FALSE(instance.is_loaded_to_memory());
	EXPECT_NO_THROW(instance.copy_referenced_section_memory());
	EXPECT_FALSE(instance.has_relocations());
	EXPECT_FALSE(instance.is_64bit());
	EXPECT_EQ(instance.strip_data_directories(0u), 0u);
	EXPECT_NO_THROW(instance.update_image_size());
	EXPECT_NO_THROW(instance.update_number_of_sections());
	EXPECT_NO_THROW(instance.update_full_headers_buffer());
}

TEST(ImageTests, LoadedToMemoryTest)
{
	pe_image instance;
	EXPECT_NO_THROW(instance.set_loaded_to_memory(true));
	EXPECT_TRUE(instance.is_loaded_to_memory());
	EXPECT_NO_THROW(instance.set_loaded_to_memory(false));
	EXPECT_FALSE(instance.is_loaded_to_memory());
}

TEST(ImageTests, Is64BitTest)
{
	pe_image instance;
	instance.get_optional_header().initialize_with<
		core::optional_header::optional_header_64_type>();
	EXPECT_TRUE(instance.is_64bit());
	instance.get_optional_header().initialize_with<
		core::optional_header::optional_header_32_type>();
	EXPECT_FALSE(instance.is_64bit());
}

TEST(ImageTests, RelocationsTest)
{
	pe_image instance;
	instance.get_data_directories().get_directories().resize(
		static_cast<std::uint32_t>(core::data_directories::directory_type::basereloc) + 1u);
	EXPECT_FALSE(instance.has_relocations());
	instance.get_data_directories().get_directory(
		core::data_directories::directory_type::basereloc).get() = { 1u, 1u };
	EXPECT_TRUE(instance.has_relocations());
	instance.get_file_header().set_characteristics(
		core::file_header::characteristics::relocs_stripped);
	EXPECT_FALSE(instance.has_relocations());
}

TEST(ImageTests, SetNumberOfDataDirectoriesTest)
{
	pe_image instance;
	EXPECT_NO_THROW(instance.set_number_of_data_directories(0u));
	EXPECT_TRUE(instance.get_data_directories().get_directories().empty());
	EXPECT_EQ(instance.get_optional_header().get_raw_number_of_rva_and_sizes(), 0u);

	expect_throw_pe_error([&instance] {
		instance.set_number_of_data_directories(
			core::optional_header::max_number_of_rva_and_sizes + 1u);
	}, image_errc::too_many_rva_and_sizes);
	EXPECT_TRUE(instance.get_data_directories().get_directories().empty());
	EXPECT_EQ(instance.get_optional_header().get_raw_number_of_rva_and_sizes(), 0u);

	EXPECT_NO_THROW(instance.set_number_of_data_directories(16u));
	EXPECT_EQ(instance.get_data_directories().get_directories().size(), 16u);
	EXPECT_EQ(instance.get_optional_header().get_raw_number_of_rva_and_sizes(), 16u);
}

TEST(ImageTests, UpdateNumberOfSectionsTest)
{
	pe_image instance;
	instance.get_section_table().get_section_headers().resize(5u);
	EXPECT_NO_THROW(instance.update_number_of_sections());
	EXPECT_EQ(instance.get_file_header().base_struct()->number_of_sections, 5u);

	instance.get_section_table().get_section_headers().resize(
		(std::numeric_limits<std::uint16_t>::max)() + 1u);
	expect_throw_pe_error([&instance] {
		instance.update_number_of_sections();
	}, image_errc::too_many_sections);
}

TEST(ImageTests, StripDataDirectories)
{
	pe_image instance;
	instance.get_optional_header().set_raw_number_of_rva_and_sizes(5u);
	instance.get_data_directories().set_size(6u);
	instance.get_data_directories().get_directory(
		core::data_directories::directory_type::exports).get() = { 1u, 1u };

	EXPECT_EQ(instance.strip_data_directories(3u), 3u);
	EXPECT_EQ(instance.get_optional_header().get_raw_number_of_rva_and_sizes(), 3u);
	EXPECT_EQ(instance.get_data_directories().get_directories().size(), 3u);

	EXPECT_EQ(instance.strip_data_directories(0u), 1u);
	EXPECT_EQ(instance.get_optional_header().get_raw_number_of_rva_and_sizes(), 1u);
	EXPECT_EQ(instance.get_data_directories().get_directories().size(), 1u);
}

TEST(ImageTests, UpdateImageSizeTest)
{
	pe_image instance;
	instance.get_optional_header().set_raw_size_of_headers(0x123u);
	EXPECT_NO_THROW(instance.update_image_size());
	EXPECT_EQ(instance.get_optional_header().get_raw_size_of_image(), 0x123u);

	instance.get_section_table().get_section_headers().emplace_back();
	auto& last_section = instance.get_section_table()
		.get_section_headers().emplace_back();
	last_section.set_rva(0x456u);
	last_section.set_virtual_size(0x789u);
	EXPECT_NO_THROW(instance.update_image_size());
	EXPECT_EQ(instance.get_optional_header().get_raw_size_of_image(),
		0x456u + 0x789u);

	last_section.set_virtual_size((std::numeric_limits<std::uint32_t>::max)());
	expect_throw_pe_error([&instance] {
		instance.update_image_size();
	}, utilities::generic_errc::integer_overflow);
}

TEST(ImageTests, CopyReferencedSectionMemoryTest)
{
	pe_image instance;
	instance.get_section_data_list().resize(3u);
	auto buffer = std::make_shared<buffers::input_container_buffer>();
	for (auto& section_data : instance.get_section_data_list()) {
		section_data.get_buffer().deserialize(buffer, false);
	}

	EXPECT_NO_THROW(instance.copy_referenced_section_memory());
	for (auto& section_data : instance.get_section_data_list()) {
		EXPECT_TRUE(section_data.get_buffer().is_copied());
	}
}

namespace
{
class ImageTestsFixture : public ::testing::TestWithParam<std::pair<bool, bool>>
{
};
} //namespace

TEST_P(ImageTestsFixture, UpdateFullHeadersBufferTest)
{
	const auto [addHeaderGapData, keepHeaderGapData] = GetParam();

	test_image_options options;
	auto instance = create_test_image(options);

	if (addHeaderGapData)
	{
		auto& full_headers_buffer = instance
			.get_full_headers_buffer().copied_data();
		full_headers_buffer.resize(
			dos::dos_header::packed_struct_type::packed_size + 100u);
		full_headers_buffer[dos::dos_header::packed_struct_type::packed_size
			+ 5u] = std::byte{ 0xcdu };
	}

	instance.get_data_directories().get_directories()
		.at(0).get() = { 0xabu, 0xcdu };
	instance.get_data_directories().get_directories()
		.at(options.number_of_data_directories - 1).get() = { 0x56u, 0x78u };
	instance.get_section_table().get_section_headers().front().set_name("abc");
	instance.get_section_table().get_section_headers().back().set_name("xyz");

	EXPECT_NO_THROW(instance.update_full_headers_buffer(keepHeaderGapData));
	const auto& buf = instance.get_full_headers_buffer();
	EXPECT_TRUE(buf.is_copied());
	const auto& data = buf.copied_data();

	auto expected_size = options.e_lfanew
		+ core::image_signature::packed_struct_type::packed_size
		+ core::file_header::packed_struct_type::packed_size
		+ instance.get_optional_header().get_size_of_structure()
		+ instance.get_data_directories().get_directories().size()
		* core::data_directories::directory_packed_size
		+ instance.get_section_table().get_section_headers().size()
		* section::section_header::packed_struct_type::packed_size;
	EXPECT_EQ(data.size(), expected_size);

	EXPECT_EQ(data[0], std::byte{ 'M' });
	EXPECT_EQ(data[1], std::byte{ 'Z' });
	EXPECT_EQ(data[options.e_lfanew], std::byte{ 'P' });
	EXPECT_EQ(data[options.e_lfanew + 1], std::byte{ 'E' });
	// Number of sections
	EXPECT_EQ(data[options.e_lfanew + sizeof(std::uint32_t)
		+ sizeof(std::uint16_t)], std::byte{3u});
	// Optional header magic
	EXPECT_EQ(data[options.e_lfanew
		+ core::image_signature::packed_struct_type::packed_size
		+ core::file_header::packed_struct_type::packed_size], std::byte{ 0x0bu });
	EXPECT_EQ(data[options.e_lfanew
		+ core::image_signature::packed_struct_type::packed_size
		+ core::file_header::packed_struct_type::packed_size + 1u], std::byte{ 0x01u });
	// First data directory
	auto first_data_dir_offset = options.e_lfanew
		+ core::image_signature::packed_struct_type::packed_size
		+ core::file_header::packed_struct_type::packed_size
		+ sizeof(std::uint16_t) //optional header magic
		+ core::optional_header::optional_header_32_type::packed_size;
	EXPECT_EQ(data[first_data_dir_offset], std::byte{ 0xabu });
	EXPECT_EQ(data[first_data_dir_offset + sizeof(std::uint32_t)], std::byte{ 0xcdu });
	// Last data directory
	auto last_data_dir_offset = first_data_dir_offset
		+ (options.number_of_data_directories - 1) * core::data_directories::directory_packed_size;
	EXPECT_EQ(data[last_data_dir_offset], std::byte{ 0x56u });
	EXPECT_EQ(data[last_data_dir_offset + sizeof(std::uint32_t)], std::byte{ 0x78u });

	auto section_table_offset = last_data_dir_offset
		+ core::data_directories::directory_packed_size;
	// Letters of first section name
	EXPECT_EQ(data[section_table_offset], std::byte{ 'a' });
	EXPECT_EQ(data[section_table_offset + 1], std::byte{ 'b' });

	auto last_section_entry_offset = section_table_offset
		+ (options.sections.size() - 1)
		* section::section_header::packed_struct_type::packed_size;
	// Letters of last section name
	EXPECT_EQ(data[last_section_entry_offset], std::byte{ 'x' });
	EXPECT_EQ(data[last_section_entry_offset + 1], std::byte{ 'y' });

	// Data between header gaps
	if (addHeaderGapData && keepHeaderGapData)
	{
		EXPECT_EQ(data[dos::dos_header::packed_struct_type::packed_size
			+ 5u], std::byte{ 0xcdu });
	}
	else
	{
		EXPECT_EQ(data[dos::dos_header::packed_struct_type::packed_size
			+ 5u], std::byte{});
	}
}

INSTANTIATE_TEST_SUITE_P(
	ImageTestsFixtureTests, ImageTestsFixture,
	::testing::Values(
		std::make_pair(false, false),
		std::make_pair(false, true),
		std::make_pair(true, false),
		std::make_pair(true, true)
	));
