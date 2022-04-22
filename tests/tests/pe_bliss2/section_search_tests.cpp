#include "gtest/gtest.h"

#include <cstdint>
#include <limits>

#include "pe_bliss2/section_search.h"
#include "pe_bliss2/section_header.h"

#include "tests/tests/pe_bliss2/pe_error_helper.h"

#include "utilities/generic_error.h"

using namespace pe_bliss;
using namespace pe_bliss::section_search;

namespace
{
constexpr auto max_offset = (std::numeric_limits<std::uint32_t>::max)();
} //namespace

TEST(SectionSearchTests, ByRawOffsetTest)
{
	section_header header;
	header.set_raw_size(512u).set_pointer_to_raw_data(1024u);

	EXPECT_FALSE(by_raw_offset(1000u, 512u)(header));
	EXPECT_TRUE(by_raw_offset(1024u, 512u)(header));
	EXPECT_TRUE(by_raw_offset(1025u, 512u)(header));
	EXPECT_TRUE(by_raw_offset(1024u + 512u, 512u)(header));

	EXPECT_FALSE(by_raw_offset(1000u, 512u, 10u)(header));
	EXPECT_TRUE(by_raw_offset(1024u, 512u, 10u)(header));
	EXPECT_TRUE(by_raw_offset(1025u, 512u, 511u)(header));
	EXPECT_FALSE(by_raw_offset(1025u, 512u, 512u)(header));
	EXPECT_TRUE(by_raw_offset(1024u + 511u, 512u, 1u)(header));
}

TEST(SectionSearchTests, ByRawRawOffsetOverflowTest)
{
	expect_throw_pe_error([&] {
		(void)by_raw_offset(max_offset, 512u, 1u); },
		utilities::generic_errc::integer_overflow);
	EXPECT_NO_THROW((void)by_raw_offset(max_offset, 512u, 0u));
}

TEST(SectionSearchTests, ByRawSectionOffsetOverflowTest)
{
	section_header header;
	header.set_raw_size(512u).set_pointer_to_raw_data(max_offset - 10u);

	EXPECT_TRUE(by_raw_offset(max_offset, 512u)(header));
	EXPECT_TRUE(by_raw_offset(max_offset - 10u, 512u)(header));
	EXPECT_FALSE(by_raw_offset(max_offset - 11u, 512u)(header));

	EXPECT_TRUE(by_raw_offset(max_offset - 10u, 512u, 10u)(header));
	EXPECT_FALSE(by_raw_offset(max_offset - 11u, 512u, 11u)(header));
}

TEST(SectionSearchTests, ByRvaTest)
{
	section_header header;
	header.set_virtual_size(512u).set_rva(1024u);

	EXPECT_FALSE(by_rva(1000u, 512u)(header));
	EXPECT_TRUE(by_rva(1024u, 512u)(header));
	EXPECT_TRUE(by_rva(1025u, 512u)(header));
	EXPECT_TRUE(by_rva(1024u + 512u, 512u)(header));

	EXPECT_FALSE(by_rva(1000u, 512u, 10u)(header));
	EXPECT_TRUE(by_rva(1024u, 512u, 10u)(header));
	EXPECT_TRUE(by_rva(1025u, 512u, 511u)(header));
	EXPECT_FALSE(by_rva(1025u, 512u, 512u)(header));
	EXPECT_TRUE(by_rva(1024u + 511u, 512u, 1u)(header));
}

TEST(SectionSearchTests, ByRvaRvaOverflowTest)
{
	expect_throw_pe_error([&] {
		(void)by_rva(max_offset, 512u, 1u); },
		utilities::generic_errc::integer_overflow);
	EXPECT_NO_THROW((void)by_rva(max_offset, 512u, 0u));
}

TEST(SectionSearchTests, ByRvaSectionRvaOverflowTest)
{
	section_header header;
	header.set_virtual_size(512u).set_rva(max_offset - 10u);

	EXPECT_TRUE(by_rva(max_offset, 512u)(header));
	EXPECT_TRUE(by_rva(max_offset - 10u, 512u)(header));
	EXPECT_FALSE(by_rva(max_offset - 11u, 512u)(header));

	EXPECT_TRUE(by_rva(max_offset - 10u, 512u, 10u)(header));
	EXPECT_FALSE(by_rva(max_offset - 11u, 512u, 11u)(header));
}

TEST(SectionSearchTests, ByPointerTest)
{
	section_header header1, header2;
	EXPECT_FALSE(by_pointer(&header1)(header2));
	EXPECT_TRUE(by_pointer(&header1)(header1));
}
