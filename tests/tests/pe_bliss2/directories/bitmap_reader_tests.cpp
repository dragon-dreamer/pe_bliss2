#include <cstddef>

#include "gtest/gtest.h"

#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_container_buffer.h"
#include "pe_bliss2/detail/resources/bitmap.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/resources/bitmap_reader.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::resources;

TEST(BitmapReaderTests, ReadEmpty)
{
	buffers::input_container_buffer buf;
	expect_throw_pe_error([&buf] {
		(void)bitmap_from_resource(buf, true);
	}, bitmap_reader_errc::invalid_buffer_size);
}

TEST(BitmapReaderTests, ReadInvalidHeader)
{
	pe_bliss::packed_struct<pe_bliss::detail::resources::bitmap_info_header> header;
	header->clr_used = 0xf0000000u;
	buffers::input_container_buffer buf;
	buf.get_container().resize(header.packed_size);
	header.serialize(buf.get_container().data(), header.packed_size, true);

	expect_throw_pe_error([&buf] {
		(void)bitmap_from_resource(buf, true);
	}, bitmap_reader_errc::invalid_bitmap_header);
}

namespace
{
void read_test(
	const pe_bliss::packed_struct<pe_bliss::detail::resources::bitmap_info_header>& header,
	std::uint32_t off_bits_delta)
{
	buffers::input_container_buffer buf;
	buf.get_container().resize(header.packed_size + 100u);
	buf.get_container().back() = std::byte{ 0xabu };
	header.serialize(buf.get_container().data(), header.packed_size, true);

	EXPECT_NO_THROW(buf.get_container() = bitmap_from_resource(buf, true));

	pe_bliss::packed_struct<pe_bliss::detail::resources::bitmap_file_header> fh;
	buffers::input_buffer_stateful_wrapper_ref ref(buf);
	EXPECT_NO_THROW(fh.deserialize(ref, true));
	EXPECT_EQ(fh->off_bits, header.packed_size + fh.packed_size + off_bits_delta);
	EXPECT_EQ(fh->type, pe_bliss::detail::resources::bm_signature);
	EXPECT_EQ(fh->size, buf.size());
	EXPECT_EQ(buf.get_container().back(), std::byte{ 0xabu });
}
} //namespace

TEST(BitmapReaderTests, Read)
{
	read_test({}, 4u);
}

TEST(BitmapReaderTests, Read2)
{
	pe_bliss::packed_struct<pe_bliss::detail::resources::bitmap_info_header> header;
	header->clr_used = 2u;
	read_test(header, 8u);
}
