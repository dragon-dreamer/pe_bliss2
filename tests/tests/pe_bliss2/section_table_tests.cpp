#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include "buffers/input_memory_buffer.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_virtual_buffer.h"
#include "buffers/output_memory_buffer.h"

#include "pe_bliss2/detail/image_section_header.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/core/optional_header.h"
#include "pe_bliss2/core/optional_header_errc.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/section/section_errc.h"
#include "pe_bliss2/section/section_header.h"
#include "pe_bliss2/section/section_table.h"
#include "pe_bliss2/section/section_table_validator.h"

#include "tests/tests/pe_bliss2/pe_error_helper.h"
#include "tests/tests/pe_bliss2/output_buffer_mock.h"

using namespace pe_bliss;
using namespace pe_bliss::core;
using namespace pe_bliss::section;

TEST(SectionTableTests, EmptyTest)
{
	section_table table;

	::testing::StrictMock<output_buffer_mock> buf;
	ASSERT_NO_THROW(table.serialize(buf, true));
	ASSERT_NO_THROW(table.serialize(buf, false));

	EXPECT_TRUE(table.get_section_headers().empty());
	EXPECT_TRUE(std::as_const(table).get_section_headers().empty());
	EXPECT_TRUE(std::move(table).get_section_headers().empty());
}

TEST(SectionTableTests, MoveTest)
{
	section_table table;
	table.get_section_headers().emplace_back();
	EXPECT_EQ(std::move(table).get_section_headers().size(), 1u);
	EXPECT_TRUE(table.get_section_headers().empty());
}

TEST(SectionTableTests, DeserializeTest1)
{
	section_table table;
	table.get_section_headers().emplace_back();
	buffers::input_memory_buffer buf(nullptr, 0);
	buffers::input_buffer_stateful_wrapper_ref ref(buf);
	EXPECT_NO_THROW(table.deserialize(ref, 0u, true));
	EXPECT_TRUE(table.get_section_headers().empty());
}

namespace pe_bliss::detail
{
bool operator==(const image_section_header& l,
	const image_section_header& r) noexcept
{
	return std::memcmp(&l, &r, sizeof(l)) == 0;
}
} //namespace pe_bliss::detail

TEST(SectionTableTests, DeserializeTest2)
{
	section_table table;
	auto buf = std::make_shared<buffers::input_memory_buffer>(nullptr, 0);
	buffers::input_virtual_buffer virtual_buf(buf, 3u
		* detail::packed_reflection::get_type_size<detail::image_section_header>());
	buffers::input_buffer_stateful_wrapper_ref ref(virtual_buf);
	ASSERT_NO_THROW(table.deserialize(ref, 3u, true));
	EXPECT_EQ(table.get_section_headers().size(), 3u);
	for (const auto& header : table.get_section_headers()) {
		EXPECT_EQ(header.base_struct().get(), detail::image_section_header{});
		EXPECT_EQ(header.base_struct().physical_size(), 0u);
	}
}

namespace
{
constexpr detail::image_section_header section_header0{
	{'t', 'e', 's', 't'}, //name
	0x78563412u, //virtual_size
	0x44332211u, //virtual_address
	0x04030201u, //size_of_raw_data
	0x08070605u, //pointer_to_raw_data
	0x0c0b0a09u, //pointer_to_relocations
	0xddccbbaau, //pointer_to_line_numbers
	0x0e0du, //number_of_relocations
	0x2b1au, //number_of_line_numbers
	0x88776655u  //characteristics
};

constexpr detail::image_section_header section_header1{
	{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'}, //name
	0x00563412u //virtual_size
};

constexpr std::array section_header_data{
	//name
	std::byte{'t'}, std::byte{'e'}, std::byte{'s'}, std::byte{'t'},
	std::byte{}, std::byte{}, std::byte{}, std::byte{},
	//virtual_size
	std::byte{0x12u}, std::byte{0x34u}, std::byte{0x56u}, std::byte{0x78u},
	//virtual_address
	std::byte{0x11u}, std::byte{0x22u}, std::byte{0x33u}, std::byte{0x44u},
	//size_of_raw_data
	std::byte{0x1u}, std::byte{0x2u}, std::byte{0x3u}, std::byte{0x4u},
	//pointer_to_raw_data
	std::byte{0x5u}, std::byte{0x6u}, std::byte{0x7u}, std::byte{0x8u},
	//pointer_to_relocations
	std::byte{0x9u}, std::byte{0xau}, std::byte{0xbu}, std::byte{0xcu},
	//pointer_to_line_numbers
	std::byte{0xaau}, std::byte{0xbbu}, std::byte{0xccu}, std::byte{0xddu},
	//number_of_relocations
	std::byte{0xdu}, std::byte{0xeu},
	//number_of_line_numbers
	std::byte{0x1au}, std::byte{0x2bu},
	//characteristics
	std::byte{0x55u}, std::byte{0x66u}, std::byte{0x77u}, std::byte{0x88u},

	//name
	std::byte{'a'}, std::byte{'b'}, std::byte{'c'}, std::byte{'d'},
	std::byte{'e'}, std::byte{'f'}, std::byte{'g'}, std::byte{'h'},
	//virtual_size
	std::byte{0x12u}, std::byte{0x34u}, std::byte{0x56u}
};
} //namespace

TEST(SectionTableTests, DeserializeTest3)
{
	section_table table;
	auto buf = std::make_shared<buffers::input_memory_buffer>(
		section_header_data.data(), section_header_data.size());
	buffers::input_virtual_buffer virtual_buf(buf, 100u);
	buffers::input_buffer_stateful_wrapper_ref ref(virtual_buf);
	ASSERT_NO_THROW(table.deserialize(ref, 3u, true));
	ASSERT_EQ(table.get_section_headers().size(), 3u);

	const auto& headers = table.get_section_headers();
	EXPECT_EQ(headers[0].base_struct().get(), section_header0);
	EXPECT_EQ(headers[1].base_struct().get(), section_header1);
	EXPECT_EQ(headers[2].base_struct().get(), detail::image_section_header{});
}

TEST(SectionTableTests, DeserializeTest4)
{
	section_table table;
	auto buf = std::make_shared<buffers::input_memory_buffer>(
		section_header_data.data(), section_header_data.size());
	buffers::input_virtual_buffer virtual_buf(buf, 100u);
	buffers::input_buffer_stateful_wrapper_ref ref(virtual_buf);

	expect_throw_pe_error([&]() {
		table.deserialize(ref, 3u, false);
	}, section_errc::unable_to_read_section_table);

	ASSERT_EQ(table.get_section_headers().size(), 2u);
	const auto& headers = table.get_section_headers();
	EXPECT_EQ(headers[0].base_struct().get(), section_header0);
}

TEST(SectionTableTests, SerializeTest1)
{
	section_table table;
	section_header header0;
	header0.base_struct() = section_header0;
	section_header header1;
	header1.base_struct() = section_header1;
	header1.base_struct().set_physical_size(section_header_data.size() -
		detail::packed_reflection::get_type_size<detail::image_section_header>());
	table.get_section_headers().emplace_back(header0);
	table.get_section_headers().emplace_back(header1);

	std::vector<std::byte> data;
	buffers::output_memory_buffer buf(data);
	ASSERT_NO_THROW(table.serialize(buf, false));
	ASSERT_EQ(data.size(), section_header_data.size());
	EXPECT_TRUE(std::equal(data.cbegin(), data.cend(),
		section_header_data.cbegin()));

	data.clear();
	buf.set_wpos(0);
	ASSERT_NO_THROW(table.serialize(buf, true));
	ASSERT_EQ(data.size(), header0.base_struct().packed_size * 2u);
	EXPECT_TRUE(std::equal(section_header_data.cbegin(),
		section_header_data.cend(), data.cbegin()));
}

TEST(SectionTableTests, SectionHeaderValidationTest1)
{
	section_header sh;
	sh.set_raw_size(section_header::two_gb_size);
	sh.set_virtual_size(section_header::two_gb_size + 1u);
	sh.set_pointer_to_raw_data(section_header::two_gb_size + 1u);
	sh.set_rva(section_header::two_gb_size);

	optional_header oh;
	oh.set_raw_file_alignment(7u);
	oh.set_raw_section_alignment(7u);

	error_list errors;
	ASSERT_NO_THROW(validate_section_header(oh, sh, 123u, errors));
	expect_contains_errors(errors, section_errc::invalid_section_raw_size,
		section_errc::invalid_section_virtual_size,
		section_errc::invalid_section_raw_address_alignment,
		section_errc::raw_section_size_overflow,
		section_errc::virtual_section_size_overflow,
		section_errc::invalid_section_virtual_address_alignment,
		section_errc::invalid_section_low_alignment);

	ASSERT_TRUE(errors.has_errors());
	for (const auto& [error, exc] : *errors.get_errors()) {
		EXPECT_EQ(exc.error, std::exception_ptr{});
		auto ctx = std::get_if<std::size_t>(&error.context);
		ASSERT_NE(ctx, nullptr);
		EXPECT_EQ(*ctx, 123u);
	}
}

TEST(SectionTableTests, SectionHeaderValidationTest2)
{
	section_table::header_list headers;
	headers.emplace_back().set_raw_size(1u);

	optional_header oh;
	oh.set_raw_section_alignment(0x200u);
	oh.set_raw_file_alignment(0x200u);

	error_list errors;
	ASSERT_NO_THROW(validate_section_header(oh,
		headers.cbegin(), headers, errors));
	EXPECT_FALSE(errors.has_any_error(section_errc::invalid_section_raw_size_alignment));

	headers.emplace_back();
	ASSERT_NO_THROW(validate_section_header(oh,
		headers.cbegin(), headers, errors));
	EXPECT_TRUE(errors.has_error(section_errc::invalid_section_raw_size_alignment, 0u));

	headers.front().set_raw_size(0x400u);
	errors.clear_errors();
	ASSERT_NO_THROW(validate_section_header(oh,
		headers.cbegin(), headers, errors));
	EXPECT_FALSE(errors.has_any_error(section_errc::invalid_section_raw_size_alignment));
}

TEST(SectionTableTests, SectionHeaderValidationTest3)
{
	section_table::header_list headers;
	headers.emplace_back().set_rva(0x1000u);

	optional_header oh;
	oh.set_raw_size_of_headers(0x1000u);

	error_list errors;
	ASSERT_NO_THROW(validate_section_header(oh,
		headers.cbegin(), headers, errors));
	EXPECT_FALSE(errors.has_error(optional_header_errc::invalid_size_of_headers));

	oh.set_raw_size_of_headers(0x1001u);
	ASSERT_NO_THROW(validate_section_header(oh,
		headers.cbegin(), headers, errors));
	EXPECT_TRUE(errors.has_error(optional_header_errc::invalid_size_of_headers));

	EXPECT_FALSE(errors.has_any_error(section_errc::virtual_gap_between_sections));
}

TEST(SectionTableTests, SectionHeaderValidationTest4)
{
	section_table::header_list headers;
	headers.emplace_back().set_rva(0x1000u).set_virtual_size(0x200u);
	headers.emplace_back().set_rva(0x1200u);

	optional_header oh;

	error_list errors;
	ASSERT_NO_THROW(validate_section_header(oh,
		headers.cbegin() + 1, headers, errors));
	EXPECT_FALSE(errors.has_any_error(section_errc::virtual_gap_between_sections));

	headers.back().set_rva(0x1200u - 1u);
	ASSERT_NO_THROW(validate_section_header(oh,
		headers.cbegin() + 1, headers, errors));
	EXPECT_TRUE(errors.has_error(section_errc::virtual_gap_between_sections, 1u));

	errors.clear_errors();
	headers.front().set_virtual_size((std::numeric_limits<rva_type>::max)());
	ASSERT_NO_THROW(validate_section_header(oh,
		headers.cbegin() + 1, headers, errors));
	EXPECT_FALSE(errors.has_any_error(section_errc::virtual_gap_between_sections));
	EXPECT_TRUE(errors.has_error(section_errc::invalid_section_virtual_size, 0u));
}

TEST(SectionTableTests, SectionHeaderValidationTest5)
{
	section_table::header_list headers;
	headers.emplace_back().set_rva(0x1000u)
		.set_virtual_size(0x200u).set_raw_size(0x123u);
	headers.emplace_back().set_rva(0x1201u).set_raw_size(0x123u);

	optional_header oh;
	oh.set_raw_file_alignment(0x200u);
	oh.set_raw_section_alignment(0x200u);

	error_list errors;
	ASSERT_NO_THROW(validate_section_headers(oh, headers, errors));
	EXPECT_TRUE(errors.has_error(section_errc::virtual_gap_between_sections, 1u));
	EXPECT_FALSE(errors.has_error(section_errc::virtual_gap_between_sections, 0u));
	EXPECT_TRUE(errors.has_error(section_errc::invalid_section_raw_size_alignment, 0u));
	EXPECT_FALSE(errors.has_error(section_errc::invalid_section_raw_size_alignment, 1u));
}

TEST(SectionTableTests, SectionSearchByRvaTest)
{
	section_table table;
	table.get_section_headers().emplace_back()
		.set_rva(0x100u).set_virtual_size(0x100u);
	table.get_section_headers().emplace_back()
		.set_rva(0x200u).set_virtual_size(0x100u);

	EXPECT_EQ(table.by_rva(0x5u, 0x100u, 0),
		table.get_section_headers().cend());
	EXPECT_EQ(std::as_const(table).by_rva(0x5u, 0x100u, 0),
		table.get_section_headers().cend());

	EXPECT_EQ(table.by_rva(0x105u, 0x100, 10),
		table.get_section_headers().cbegin());
	EXPECT_EQ(std::as_const(table).by_rva(0x105u, 0x100, 10),
		table.get_section_headers().cbegin());

	EXPECT_EQ(table.by_rva(0x205u, 0x100, 10),
		table.get_section_headers().cbegin() + 1);
	EXPECT_EQ(std::as_const(table).by_rva(0x205u, 0x100, 10),
		table.get_section_headers().cbegin() + 1);
}

TEST(SectionTableTests, SectionSearchByRawOffsetTest)
{
	section_table table;
	table.get_section_headers().emplace_back()
		.set_pointer_to_raw_data(0x1000u).set_raw_size(0x100u);
	table.get_section_headers().emplace_back()
		.set_pointer_to_raw_data(0x2000u).set_raw_size(0x100u);

	EXPECT_EQ(table.by_raw_offset(0x5u, 0x100u, 0),
		table.get_section_headers().cend());
	EXPECT_EQ(std::as_const(table).by_raw_offset(0x5u, 0x100u, 0),
		table.get_section_headers().cend());

	EXPECT_EQ(table.by_raw_offset(0x1000u, 0x100, 10),
		table.get_section_headers().cbegin());
	EXPECT_EQ(std::as_const(table).by_raw_offset(0x1000u, 0x100, 10),
		table.get_section_headers().cbegin());

	EXPECT_EQ(table.by_raw_offset(0x2000u, 0x100, 10),
		table.get_section_headers().cbegin() + 1);
	EXPECT_EQ(std::as_const(table).by_raw_offset(0x2000u, 0x100, 10),
		table.get_section_headers().cbegin() + 1);
}

TEST(SectionTableTests, SectionSearchByReferenceTest)
{
	section_table table;
	table.get_section_headers().emplace_back();
	table.get_section_headers().emplace_back();

	EXPECT_EQ(table.by_reference(table.get_section_headers().back()),
		table.get_section_headers().cbegin() + 1);
	EXPECT_EQ(std::as_const(table).by_reference(table.get_section_headers().back()),
		table.get_section_headers().cbegin() + 1);

	EXPECT_EQ(table.by_reference(table.get_section_headers().front()),
		table.get_section_headers().cbegin());
	EXPECT_EQ(std::as_const(table).by_reference(table.get_section_headers().front()),
		table.get_section_headers().cbegin());

	section_header header;
	EXPECT_EQ(table.by_reference(header),
		table.get_section_headers().cend());
	EXPECT_EQ(std::as_const(table).by_reference(header),
		table.get_section_headers().cend());
}

TEST(SectionTableTests, GetRawDataEndOffsetTest)
{
	section_table table;
	EXPECT_EQ(table.get_raw_data_end_offset(0x1000), 0u);

	auto& s1 = table.get_section_headers().emplace_back();
	s1.base_struct()->size_of_raw_data = 0x200u;
	s1.base_struct()->pointer_to_raw_data = 0x800u;
	auto& s2 = table.get_section_headers().emplace_back();
	s2.base_struct()->size_of_raw_data = 0x200u;
	s2.base_struct()->pointer_to_raw_data = 0x400u;

	EXPECT_EQ(table.get_raw_data_end_offset(0x1000), 0xa00u);

	s2.base_struct()->size_of_raw_data
		= (std::numeric_limits<std::uint32_t>::max)() - 1u;
	s2.base_struct()->pointer_to_raw_data = s2.base_struct()->size_of_raw_data;

	std::uint64_t expected = (std::numeric_limits<std::uint32_t>::max)() - 1u;
	expected *= 2u;
	EXPECT_EQ(table.get_raw_data_end_offset(0x1000), expected);
}
