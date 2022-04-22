#include <array>
#include <cstddef>
#include <memory>

#include "gtest/gtest.h"

#include "buffers/buffer_copy.h"
#include "buffers/input_stream_buffer.h"
#include "buffers/output_stream_buffer.h"
#include "tests/tests/buffers/buffer_helpers.h"

namespace
{
class BufferTestsFixture : public ::testing::TestWithParam<std::size_t> {
};
} //namespace

TEST_P(BufferTestsFixture, BufferCopyTest)
{
	auto src = create_stream(GetParam());
	std::stringstream dst;
	buffers::input_stream_buffer src_buf(src);
	buffers::output_stream_buffer dst_buf(dst);
	EXPECT_EQ(buffers::copy(src_buf, dst_buf, GetParam()), GetParam());
	EXPECT_EQ(src->view(), dst.view());
}

INSTANTIATE_TEST_SUITE_P(
	BufferCopyTest,
	BufferTestsFixture,
	::testing::Values(
		0u, 10u, 512u, 1000u, 1024u, 1050u
	));

TEST(BufferTests, BufferOverCopyTest)
{
	auto src = create_stream(600u);
	std::stringstream dst;
	buffers::input_stream_buffer src_buf(src);
	buffers::output_stream_buffer dst_buf(dst);
	EXPECT_EQ(buffers::copy(src_buf, dst_buf, src->view().size() + 100u),
		src->view().size());
	EXPECT_EQ(src->view(), dst.view());
}
