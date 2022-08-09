#include "gtest/gtest.h"

#include <cstdint>
#include <iterator>

#include "buffers/input_memory_buffer.h"
#include "buffers/input_buffer_stateful_wrapper.h"

#include "pe_bliss2/section/section_errc.h"
#include "pe_bliss2/section/section_header.h"
#include "pe_bliss2/section/section_header_validator.h"
#include "pe_bliss2/section/section_table.h"

#include "tests/tests/pe_bliss2/pe_error_helper.h"

#include "utilities/generic_error.h"

using namespace pe_bliss::section;

TEST(SectionHeaderTests, EmptySectionHeaderTest)
{
	section_header header;
	EXPECT_TRUE(header.get_name().empty());
	EXPECT_EQ(header.get_characteristics(),
		section_header::characteristics::value{});
	EXPECT_EQ(header.get_raw_size(512u), 0u);
	EXPECT_EQ(header.get_virtual_size(512u), 0u);
	EXPECT_EQ(header.get_pointer_to_raw_data(), 0u);
	EXPECT_EQ(header.get_rva(), 0u);
	EXPECT_TRUE(header.empty());
	EXPECT_FALSE(validate_raw_size(header, 512u));
	EXPECT_TRUE(validate_virtual_size(header));
	EXPECT_TRUE(validate_raw_address(header));

	EXPECT_FALSE(validate_raw_size_alignment(header, 512, 512, false));
	EXPECT_FALSE(validate_raw_size_alignment(header, 512, 512, true));
	EXPECT_TRUE(validate_raw_size_alignment(header, 511, 512, false));
	EXPECT_TRUE(validate_raw_size_alignment(header, 511, 512, true));
	EXPECT_TRUE(validate_raw_size_alignment(header, 512, 511, false));
	EXPECT_TRUE(validate_raw_size_alignment(header, 512, 511, true));
	EXPECT_TRUE(validate_raw_size_alignment(header, 0, 0, false));
	EXPECT_TRUE(validate_raw_size_alignment(header, 0, 0, true));

	EXPECT_TRUE(validate_raw_address_alignment(header, 0));
	EXPECT_FALSE(validate_raw_address_alignment(header, 512));
	EXPECT_TRUE(validate_raw_address_alignment(header, 511));

	EXPECT_TRUE(validate_virtual_address_alignment(header, 0));
	EXPECT_FALSE(validate_virtual_address_alignment(header, 512));
	EXPECT_TRUE(validate_virtual_address_alignment(header, 511));

	EXPECT_TRUE(header.is_low_alignment());

	EXPECT_FALSE(validate_raw_size_bounds(header, 512));
	EXPECT_FALSE(validate_raw_size_bounds(header, 0));

	EXPECT_FALSE(validate_virtual_size_bounds(header, 512));
	EXPECT_FALSE(validate_virtual_size_bounds(header, 0));

	EXPECT_FALSE(header.is_writable());
	EXPECT_FALSE(header.is_readable());
	EXPECT_FALSE(header.is_executable());
	EXPECT_FALSE(header.is_shared());
	EXPECT_FALSE(header.is_discardable());
	EXPECT_TRUE(header.is_pageable());
	EXPECT_TRUE(header.is_cacheable());

	expect_throw_pe_error([&] {
		(void)header.rva_from_section_offset(1, 512); },
		section_errc::invalid_section_offset);

	expect_throw_pe_error([&] { header.set_virtual_size(0); },
		section_errc::invalid_section_virtual_size);

	EXPECT_NO_THROW(header.set_virtual_size(100));
	EXPECT_FALSE(validate_virtual_size(header));
	EXPECT_EQ(header.get_virtual_size(1u), 100u);
	EXPECT_EQ(header.get_virtual_size(0u), 100u);
	EXPECT_EQ(header.get_virtual_size(64u), 128u);
	expect_throw_pe_error([&] {
		(void)header.rva_from_section_offset(101, 1u); },
		section_errc::invalid_section_offset);
	EXPECT_EQ(header.rva_from_section_offset(80, 2u), 80u);
	EXPECT_EQ(header.rva_from_section_offset(123, 512), 123u);
}

TEST(SectionHeaderTests, SectionHeaderNameTest1)
{
	section_header header;
	header.base_struct()->name[0] = static_cast<std::uint8_t>('a');
	EXPECT_EQ(header.get_name(), "a");
	header.base_struct()->name[std::size(header.base_struct()->name) - 1]
		= static_cast<std::uint8_t>('b');

	std::string name(std::size(header.base_struct()->name), '\0');
	name[0] = 'a';
	name.back() = 'b';
	EXPECT_EQ(header.get_name(), name);

	header.base_struct()->name[1]
		= static_cast<std::uint8_t>('b');
	header.base_struct()->name[2]
		= static_cast<std::uint8_t>('c');
	header.base_struct()->name[std::size(header.base_struct()->name) - 1]
		= 0;
	EXPECT_EQ(header.get_name(), "abc");
}

TEST(SectionHeaderTests, SectionHeaderNameTest2)
{
	section_header header;
	static constexpr const char name[] = "123\0456";
	EXPECT_EQ(&header.set_name({ std::begin(name), std::size(name) - 1 }),
		&header);
	EXPECT_EQ(header.get_name(), std::string(std::begin(name),
		std::prev(std::end(name))));

	expect_throw_pe_error([&] {
		header.set_name("123456789"); },
		utilities::generic_errc::buffer_overrun);
	EXPECT_EQ(header.get_name(), std::string(std::begin(name),
		std::prev(std::end(name))));
}

TEST(SectionHeaderTests, CharacteristicsTest)
{
	section_header header;

	EXPECT_FALSE(header.is_writable());
	EXPECT_FALSE(header.is_readable());
	EXPECT_FALSE(header.is_executable());
	EXPECT_FALSE(header.is_shared());
	EXPECT_FALSE(header.is_discardable());
	EXPECT_TRUE(header.is_pageable());
	EXPECT_TRUE(header.is_cacheable());

	EXPECT_EQ(&header.set_discardable(true), &header);
	EXPECT_FALSE(header.is_writable());
	EXPECT_FALSE(header.is_readable());
	EXPECT_FALSE(header.is_executable());
	EXPECT_FALSE(header.is_shared());
	EXPECT_TRUE(header.is_discardable());
	EXPECT_TRUE(header.is_pageable());
	EXPECT_TRUE(header.is_cacheable());

	EXPECT_EQ(&header.set_discardable(false), &header);
	EXPECT_EQ(&header.set_executable(true), &header);
	EXPECT_FALSE(header.is_writable());
	EXPECT_FALSE(header.is_readable());
	EXPECT_TRUE(header.is_executable());
	EXPECT_FALSE(header.is_shared());
	EXPECT_FALSE(header.is_discardable());
	EXPECT_TRUE(header.is_pageable());
	EXPECT_TRUE(header.is_cacheable());

	EXPECT_EQ(&header.set_executable(false), &header);
	EXPECT_EQ(&header.set_non_cacheable(true), &header);
	EXPECT_FALSE(header.is_writable());
	EXPECT_FALSE(header.is_readable());
	EXPECT_FALSE(header.is_executable());
	EXPECT_FALSE(header.is_shared());
	EXPECT_FALSE(header.is_discardable());
	EXPECT_TRUE(header.is_pageable());
	EXPECT_FALSE(header.is_cacheable());

	EXPECT_EQ(&header.set_non_cacheable(false), &header);
	EXPECT_EQ(&header.set_non_pageable(true), &header);
	EXPECT_FALSE(header.is_writable());
	EXPECT_FALSE(header.is_readable());
	EXPECT_FALSE(header.is_executable());
	EXPECT_FALSE(header.is_shared());
	EXPECT_FALSE(header.is_discardable());
	EXPECT_FALSE(header.is_pageable());
	EXPECT_TRUE(header.is_cacheable());

	EXPECT_EQ(&header.set_non_pageable(false), &header);
	EXPECT_EQ(&header.set_readable(true), &header);
	EXPECT_FALSE(header.is_writable());
	EXPECT_TRUE(header.is_readable());
	EXPECT_FALSE(header.is_executable());
	EXPECT_FALSE(header.is_shared());
	EXPECT_FALSE(header.is_discardable());
	EXPECT_TRUE(header.is_pageable());
	EXPECT_TRUE(header.is_cacheable());
	
	EXPECT_EQ(&header.set_readable(false), &header);
	EXPECT_EQ(&header.set_shared(true), &header);
	EXPECT_FALSE(header.is_writable());
	EXPECT_FALSE(header.is_readable());
	EXPECT_FALSE(header.is_executable());
	EXPECT_TRUE(header.is_shared());
	EXPECT_FALSE(header.is_discardable());
	EXPECT_TRUE(header.is_pageable());
	EXPECT_TRUE(header.is_cacheable());

	EXPECT_EQ(&header.set_shared(false), &header);
	EXPECT_EQ(&header.set_writable(true), &header);
	EXPECT_TRUE(header.is_writable());
	EXPECT_FALSE(header.is_readable());
	EXPECT_FALSE(header.is_executable());
	EXPECT_FALSE(header.is_shared());
	EXPECT_FALSE(header.is_discardable());
	EXPECT_TRUE(header.is_pageable());
	EXPECT_TRUE(header.is_cacheable());

	EXPECT_EQ(header.base_struct()->characteristics,
		section_header::characteristics::mem_write);

	EXPECT_EQ(header.get_characteristics(),
		section_header::characteristics::mem_write);

	static const auto characteristics =
		static_cast<section_header::characteristics::value>(
			section_header::characteristics::mem_write
			| section_header::characteristics::mem_read);
	EXPECT_EQ(&header.set_characteristics(characteristics), &header);
	EXPECT_EQ(header.get_characteristics(), characteristics);
}

TEST(SectionHeaderTests, VirtualSizeTest1)
{
	section_header header;
	EXPECT_TRUE(validate_virtual_size(header));
	expect_throw_pe_error([&] { header.set_virtual_size(0u); },
		section_errc::invalid_section_virtual_size);
	EXPECT_EQ(&header.set_virtual_size(123u), &header);
	EXPECT_TRUE(header.empty());
	EXPECT_EQ(header.get_virtual_size(0u), 123u);
	EXPECT_EQ(header.get_virtual_size(2u), 124u);
	EXPECT_FALSE(validate_virtual_size(header));

	EXPECT_EQ(&header.set_virtual_size(0xf0000000u), &header);
	EXPECT_TRUE(validate_virtual_size(header));

	EXPECT_FALSE(validate_virtual_size_bounds(header, 512u));
	EXPECT_EQ(&header.set_rva(0xf0000000u), &header);
	EXPECT_TRUE(validate_virtual_size_bounds(header, 512u));
}

TEST(SectionHeaderTests, VirtualSizeTest2)
{
	section_header header;
	EXPECT_EQ(&header.set_raw_size(123u), &header);
	EXPECT_EQ(header.get_virtual_size(0u), 123u);
	EXPECT_EQ(&header.set_virtual_size(0u), &header);
	EXPECT_EQ(header.get_virtual_size(0u), 123u);
}

TEST(SectionHeaderTests, VirtualSizeTest3)
{
	section_header header;
	header.set_raw_size(123u);
	EXPECT_FALSE(validate_virtual_size(header));
}

TEST(SectionHeaderTests, RawSizeTest1)
{
	section_header header;
	EXPECT_FALSE(validate_raw_size(header, 512u));
	EXPECT_EQ(header.get_raw_size(0u), 0u);
	EXPECT_TRUE(header.empty());
	EXPECT_EQ(&header.set_raw_size(123u), &header);
	EXPECT_FALSE(header.empty());
	EXPECT_EQ(header.get_raw_size(0u), 123u);
	EXPECT_EQ(header.get_raw_size(512u), 123u);
	EXPECT_FALSE(validate_raw_size(header, 512u));
	EXPECT_EQ(&header.set_raw_size(0xf0000000u), &header);
	EXPECT_TRUE(validate_raw_size(header, 512u));

	EXPECT_FALSE(validate_raw_size_bounds(header, 512u));
	EXPECT_EQ(&header.set_pointer_to_raw_data(0xf0000000), &header);
	EXPECT_TRUE(validate_raw_size_bounds(header, 512u));
}

TEST(SectionHeaderTests, RawSizeTest2)
{
	section_header header;
	EXPECT_EQ(&header.set_raw_size(1000u), &header);
	EXPECT_EQ(&header.set_virtual_size(500u), &header);
	EXPECT_EQ(header.get_raw_size(512u), 512u);
}

TEST(SectionHeaderTests, PointerToRawDataTest1)
{
	section_header header;
	EXPECT_TRUE(validate_raw_address(header));
	EXPECT_EQ(&header.set_raw_size(1000u), &header);
	EXPECT_EQ(&header.set_pointer_to_raw_data(128u), &header);
	EXPECT_FALSE(validate_raw_address(header));

	EXPECT_TRUE(validate_raw_address_alignment(header, 0u));
	EXPECT_FALSE(validate_raw_address_alignment(header, 1u));
	EXPECT_FALSE(validate_raw_address_alignment(header, 64u));
	EXPECT_FALSE(validate_raw_address_alignment(header, 1024u));

	EXPECT_TRUE(validate_raw_size_alignment(header, 512u, 512u, false));
	EXPECT_FALSE(validate_raw_size_alignment(header, 512u, 512u, true));

	EXPECT_EQ(&header.set_pointer_to_raw_data(
		section_header::max_raw_address_rounded_to_0 + 1), &header);
	EXPECT_TRUE(validate_raw_address_alignment(header, 0u));
	EXPECT_FALSE(validate_raw_address_alignment(header, 1u));
	EXPECT_FALSE(validate_raw_address_alignment(header, 64u));
	EXPECT_TRUE(validate_raw_address_alignment(header, 1024u));
}

TEST(SectionHeaderTests, PointerToRawDataTest2)
{
	section_header header;
	EXPECT_EQ(&header.set_virtual_size(512u), &header);
	EXPECT_FALSE(validate_raw_address(header));
	EXPECT_FALSE(validate_raw_address_alignment(header, 64u));
}

TEST(SectionHeaderTests, RvaTest)
{
	section_header header;
	EXPECT_EQ(&header.set_rva(128u), &header);
	EXPECT_EQ(header.get_rva(), 128u);
	EXPECT_TRUE(validate_virtual_address_alignment(header, 123u));
	EXPECT_TRUE(validate_virtual_address_alignment(header, 512u));
	EXPECT_FALSE(validate_virtual_address_alignment(header, 1u));
	EXPECT_FALSE(validate_virtual_address_alignment(header, 64u));
}

TEST(SectionHeaderTests, LowAlignmentTest)
{
	section_header header;
	EXPECT_EQ(&header.set_rva(128u), &header);
	EXPECT_EQ(&header.set_pointer_to_raw_data(1024u), &header);
	EXPECT_FALSE(header.is_low_alignment());
	EXPECT_EQ(&header.set_rva(1024u), &header);
	EXPECT_TRUE(header.is_low_alignment());
}

TEST(SectionHeaderTests, RvaFromSectionOffsetTest)
{
	section_header header;
	header.set_virtual_size(512u);
	header.set_rva(2048u);
	EXPECT_EQ(header.rva_from_section_offset(2u, 512u), 2050u);
	EXPECT_EQ(header.rva_from_section_offset(511u, 512u), 2559u);
	expect_throw_pe_error([&] {
		(void)header.rva_from_section_offset(512u, 512u); },
		section_errc::invalid_section_offset);
}

TEST(SectionHeaderTests, DeserializeErrorTest)
{
	section_header header;
	buffers::input_memory_buffer buf(
		reinterpret_cast<const std::byte*>(""), 0u);
	buffers::input_buffer_stateful_wrapper_ref ref(buf);
	expect_throw_pe_error(
		[&header, &ref] { header.deserialize(ref, false); },
		section_errc::unable_to_read_section_table);
}

// Serialize calls base_struct().serialize()
// and thus does not require special tests
