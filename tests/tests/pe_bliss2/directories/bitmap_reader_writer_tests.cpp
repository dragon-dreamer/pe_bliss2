#include <cstddef>
#include <memory>

#include "gtest/gtest.h"

#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_container_buffer.h"
#include "buffers/output_memory_buffer.h"
#include "pe_bliss2/detail/resources/bitmap.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/resources/bitmap.h"
#include "pe_bliss2/resources/bitmap_reader.h"
#include "pe_bliss2/resources/bitmap_writer.h"
#include "pe_bliss2/resources/resource_reader_errc.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::resources;

TEST(BitmapReaderWriterTests, ReadEmpty)
{
	auto buf = std::make_shared<buffers::input_container_buffer>();
	expect_throw_pe_error([&buf] {
		(void)bitmap_from_resource(buf);
	}, resource_reader_errc::invalid_buffer_size);
}

TEST(BitmapReaderWriterTests, ReadInvalidHeader)
{
	pe_bliss::packed_struct<pe_bliss::detail::resources::bitmap_info_header> header;
	header->clr_used = 0xf0000000u;
	auto buf = std::make_shared<buffers::input_container_buffer>();
	buf->get_container().resize(header.packed_size);
	header.serialize(buf->get_container().data(), header.packed_size, true);

	expect_throw_pe_error([&buf] {
		(void)bitmap_from_resource(buf);
	}, bitmap_reader_errc::invalid_bitmap_header);
}

namespace
{
void read_write_test(
	const pe_bliss::packed_struct<pe_bliss::detail::resources::bitmap_info_header>& header,
	std::uint32_t off_bits_delta)
{
	auto buf = std::make_shared<buffers::input_container_buffer>();
	buf->get_container().resize(header.packed_size + 100u);
	buf->get_container().back() = std::byte{ 0xabu };
	header.serialize(buf->get_container().data(), header.packed_size, true);

	bitmap bmp;
	EXPECT_NO_THROW(bmp = bitmap_from_resource(buf));

	EXPECT_EQ(bmp.get_file_header()->off_bits,
		header.packed_size + bitmap::file_header_type::packed_size + off_bits_delta);
	EXPECT_EQ(bmp.get_file_header()->type, pe_bliss::detail::resources::bm_signature);
	EXPECT_EQ(bmp.get_info_header()->clr_used, header->clr_used);
	EXPECT_FALSE(bmp.get_buffer().is_copied());

	{
		buffers::output_memory_buffer::buffer_type vec;
		buffers::output_memory_buffer out(vec);
		EXPECT_NO_THROW(write_bitmap(bmp, out, { .mode = bitmap_serialization_mode::resource }));
		EXPECT_EQ(buf->get_container(), vec);
	}
	{
		buffers::output_memory_buffer::buffer_type vec;
		buffers::output_memory_buffer out(vec);
		EXPECT_NO_THROW(write_bitmap(bmp, out, {}));

		buf->get_container() = std::move(vec);
		pe_bliss::packed_struct<pe_bliss::detail::resources::bitmap_file_header> fh;
		buffers::input_buffer_stateful_wrapper_ref ref(*buf);
		EXPECT_NO_THROW(fh.deserialize(ref, true));
		EXPECT_EQ(fh->off_bits, header.packed_size + fh.packed_size + off_bits_delta);
		EXPECT_EQ(fh->type, pe_bliss::detail::resources::bm_signature);
		EXPECT_EQ(fh->size, buf->size());
		EXPECT_EQ(buf->get_container().back(), std::byte{ 0xabu });
	}
}
} //namespace

TEST(BitmapReaderWriterTests, ReadWrite)
{
	read_write_test({}, 4u);
}

TEST(BitmapReaderWriterTests, ReadWrite2)
{
	pe_bliss::packed_struct<pe_bliss::detail::resources::bitmap_info_header> header;
	header->clr_used = 2u;
	read_write_test(header, 8u);
}
