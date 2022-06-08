#include "gtest/gtest.h"

#include <tuple>
#include <utility>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_section_search.h"
#include "pe_bliss2/section/section_header.h"

#include "tests/tests/pe_bliss2/image_helper.h"

namespace
{
std::tuple<pe_bliss::section::section_table::header_list::iterator,
	pe_bliss::section::section_table::header_list::const_iterator,
	pe_bliss::section::section_data_list::iterator,
	pe_bliss::section::section_data_list::const_iterator>
	get_end_section_iterators(pe_bliss::image::image& instance)
{
	auto headers_end = instance.get_section_table().get_section_headers().end();
	auto data_end = instance.get_section_data_list().end();
	auto headers_cend = instance.get_section_table().get_section_headers().cend();
	auto data_cend = instance.get_section_data_list().cend();
	return { headers_end, headers_cend, data_end, data_cend };
}

std::tuple<pe_bliss::section::section_table::header_list::iterator,
	pe_bliss::section::section_table::header_list::const_iterator,
	pe_bliss::section::section_data_list::iterator,
	pe_bliss::section::section_data_list::const_iterator>
	get_begin_section_iterators(pe_bliss::image::image& instance)
{
	auto headers_begin = instance.get_section_table().get_section_headers().begin();
	auto data_begin = instance.get_section_data_list().begin();
	auto headers_cbegin = instance.get_section_table().get_section_headers().cbegin();
	auto data_cbegin = instance.get_section_data_list().cbegin();
	return { headers_begin, headers_cbegin, data_begin, data_cbegin };
}
} //namespace

TEST(ImageSectionSearchTests, SectionFromReferenceTest)
{
	auto instance = create_test_image({});
	auto [headers_end, headers_cend, data_end, data_cend]
		= get_end_section_iterators(instance);

	pe_bliss::section::section_header dummy;
	EXPECT_EQ(section_from_reference(instance, dummy), std::pair(headers_end, data_end));
	EXPECT_EQ(section_from_reference(std::as_const(instance), dummy),
		std::pair(headers_cend, data_cend));

	auto [headers_begin, headers_cbegin, data_begin, data_cbegin]
		= get_begin_section_iterators(instance);
	EXPECT_EQ(section_from_reference(instance, *headers_begin),
		std::pair(headers_begin, data_begin));
	EXPECT_EQ(section_from_reference(std::as_const(instance), *headers_cbegin),
		std::pair(headers_cbegin, data_cbegin));
}

TEST(ImageSectionSearchTests, SectionFromRvaTest)
{
	auto instance = create_test_image({});
	auto [headers_end, headers_cend, data_end, data_cend]
		= get_end_section_iterators(instance);

	EXPECT_EQ(section_from_rva(instance, 0, 0), std::pair(headers_end, data_end));
	EXPECT_EQ(section_from_rva(std::as_const(instance), 0, 0),
		std::pair(headers_cend, data_cend));
	EXPECT_EQ(section_from_rva(instance, 0x5000u, 0),
		std::pair(headers_end - 1, data_end - 1));
	EXPECT_EQ(section_from_rva(instance, 0x4000u, 0),
		std::pair(headers_end - 2, data_end - 2));
	EXPECT_EQ(section_from_rva(instance, 0x4000u, 1),
		std::pair(headers_end - 1, data_end - 1));

	EXPECT_EQ(section_from_rva(instance, 0x4000u, 0x3000u),
		std::pair(headers_end - 1, data_end - 1));
	EXPECT_EQ(section_from_rva(instance, 0x4000u, 0x3001u),
		std::pair(headers_end, data_end));
}

TEST(ImageSectionSearchTests, SectionFromVaTest)
{
	test_image_options options;
	auto instance = create_test_image(options);
	auto [headers_end, headers_cend, data_end, data_cend]
		= get_end_section_iterators(instance);

	EXPECT_EQ(section_from_va(instance, 0u, 0u),
		std::pair(headers_end, data_end));
	EXPECT_EQ(section_from_va(std::as_const(instance), 0u, 0u),
		std::pair(headers_cend, data_cend));

	EXPECT_EQ(section_from_va(instance, options.image_base, 0u),
		std::pair(headers_end, data_end));
	
	EXPECT_EQ(section_from_va(instance, options.image_base + 0x5000u, 0),
		std::pair(headers_end - 1, data_end - 1));
	EXPECT_EQ(section_from_va(instance, options.image_base + 0x4000u, 0),
		std::pair(headers_end - 2, data_end - 2));
	EXPECT_EQ(section_from_va(instance, options.image_base + 0x4000u, 1),
		std::pair(headers_end - 1, data_end - 1));

	EXPECT_EQ(section_from_va(instance, options.image_base + 0x4000u, 0x3000u),
		std::pair(headers_end - 1, data_end - 1));
	EXPECT_EQ(section_from_va(instance, options.image_base + 0x4000u, 0x3001u),
		std::pair(headers_end, data_end));
}

TEST(ImageSectionSearchTests, SectionFromDirectoryTest)
{
	auto instance = create_test_image({});
	auto [headers_end, headers_cend, data_end, data_cend]
		= get_end_section_iterators(instance);

	EXPECT_EQ(section_from_directory(instance,
		pe_bliss::core::data_directories::directory_type::imports),
		std::pair(headers_end, data_end));
	EXPECT_EQ(section_from_directory(std::as_const(instance),
		pe_bliss::core::data_directories::directory_type::imports),
		std::pair(headers_cend, data_cend));

	instance.get_data_directories().set_size(16u);
	instance.get_data_directories().get_directory(
		pe_bliss::core::data_directories::directory_type::imports) = { 0x3500u, 0x100u };

	EXPECT_EQ(section_from_directory(instance,
		pe_bliss::core::data_directories::directory_type::imports),
		std::pair(headers_end - 2, data_end - 2));
}

TEST(ImageSectionSearchTests, SectionFromFileOffsetTest)
{
	auto instance = create_test_image({ .start_section_raw_offset = 0x10000u });
	auto [headers_end, headers_cend, data_end, data_cend]
		= get_end_section_iterators(instance);

	EXPECT_EQ(section_from_file_offset(instance, 0u, 1u),
		std::pair(headers_end, data_end));
	EXPECT_EQ(section_from_file_offset(std::as_const(instance), 0u, 1u),
		std::pair(headers_cend, data_cend));

	EXPECT_EQ(section_from_file_offset(instance, 0u, 0u),
		std::pair(headers_end - 1, data_end - 1));

	EXPECT_EQ(section_from_file_offset(instance, 0x10000u, 0u),
		std::pair(headers_end - 3, data_end - 3));
	EXPECT_EQ(section_from_file_offset(instance, 0x10000u, 0x1000u),
		std::pair(headers_end - 3, data_end - 3));
	EXPECT_EQ(section_from_file_offset(instance, 0x10000u, 0x1001u),
		std::pair(headers_end, data_end));
}
