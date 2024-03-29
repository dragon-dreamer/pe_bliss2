#include "gtest/gtest.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

#include "buffers/input_memory_buffer.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_virtual_buffer.h"
#include "buffers/output_memory_buffer.h"

#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/detail/image_file_header.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/pe_error.h"

#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::core;

TEST(FileHeaderTests, EmptyTest)
{
	file_header header;
	EXPECT_EQ(header.get_characteristics(), 0u);
	EXPECT_EQ(header.get_machine_type(),
		static_cast<file_header::machine_type>(0u));
	EXPECT_EQ(header.get_section_table_buffer_pos(),
		detail::packed_reflection::get_type_size<detail::image_file_header>());
}

TEST(FileHeaderTests, GetSetTest)
{
	file_header header;
	header.set_characteristics(file_header::characteristics::dll);
	EXPECT_EQ(header.get_characteristics(),
		file_header::characteristics::dll);
	header.set_machine_type(file_header::machine_type::i386);
	EXPECT_EQ(header.get_machine_type(),
		file_header::machine_type::i386);
}

TEST(FileHeaderTests, SectionTableStartTest)
{
	file_header header;
	header.get_descriptor()->size_of_optional_header = 0xe0u;
	header.get_descriptor().get_state().set_buffer_pos(0x123u);
	EXPECT_EQ(header.get_section_table_buffer_pos(), 0xe0u + 0x123u
		+ detail::packed_reflection::get_type_size<detail::image_file_header>());
}

TEST(FileHeaderTests, DeserializeSerializeTest)
{
	static constexpr std::byte data[]{
		std::byte{0x4c}, std::byte{0x01}, //machine
		std::byte{0x50}, std::byte{0x02}, //number_of_sections
		std::byte{0x01}, std::byte{0x02}, //time_date_stamp
		std::byte{0x03}, std::byte{0x04},
		std::byte{0x05}, std::byte{0x06}, //pointer_to_symbol_table
		std::byte{0x07}, std::byte{0x08},
		std::byte{0x09}, std::byte{0x0a}, //number_of_symbols
		std::byte{0x0b}, std::byte{0x0c},
		std::byte{0x56}, std::byte{0x78}, //size_of_optional_header
		std::byte{0x02} //characteristics (last byte cut, virtual)
	};

	auto buf = std::make_shared<buffers::input_memory_buffer>(data, sizeof(data));
	buffers::input_virtual_buffer virtual_buf(buf, 1u);
	buffers::input_buffer_stateful_wrapper_ref ref(virtual_buf);

	file_header header;
	expect_throw_pe_error([&header, &ref] {
		header.deserialize(ref, false);
	}, file_header_errc::unable_to_read_file_header);
	ref.set_rpos(0u);

	ASSERT_NO_THROW(header.deserialize(ref, true));
	EXPECT_EQ(header.get_machine_type(), file_header::machine_type::i386);
	EXPECT_EQ(header.get_descriptor()->number_of_sections, 0x250u);
	EXPECT_EQ(header.get_descriptor()->time_date_stamp, 0x04030201u);
	EXPECT_EQ(header.get_descriptor()->pointer_to_symbol_table, 0x08070605u);
	EXPECT_EQ(header.get_descriptor()->number_of_symbols, 0x0c0b0a09u);
	EXPECT_EQ(header.get_descriptor()->size_of_optional_header, 0x7856u);
	EXPECT_EQ(header.get_descriptor()->characteristics,
		file_header::characteristics::executable_image);

	std::vector<std::byte> outdata;
	buffers::output_memory_buffer outbuf(outdata);
	ASSERT_NO_THROW(header.serialize(outbuf, false));
	ASSERT_EQ(outdata.size(), sizeof(data));
	EXPECT_TRUE(std::equal(outdata.cbegin(), outdata.cend(), data));
}
