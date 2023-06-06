#include <array>
#include <cstddef>
#include <memory>
#include <system_error>

#include "gtest/gtest.h"

#include "buffers/buffer_copy.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_stream_buffer.h"
#include "buffers/input_virtual_buffer.h"
#include "buffers/output_stream_buffer.h"
#include "tests/buffers/buffer_helpers.h"

namespace
{
class BufferTestsFixture : public ::testing::TestWithParam<std::size_t>
{
};
} //namespace

TEST_P(BufferTestsFixture, BufferCopyTest)
{
	auto src = create_stream(GetParam());
	std::stringstream dst;
	auto src_buf = std::make_shared<buffers::input_stream_buffer>(src);
	buffers::output_stream_buffer dst_buf(dst);
	buffers::input_buffer_stateful_wrapper wrapper(src_buf);
	ASSERT_EQ(buffers::copy(wrapper, dst_buf, GetParam()), GetParam());
	EXPECT_EQ(src->view(), dst.view());
}

TEST_P(BufferTestsFixture, BufferCopyVirtualTest)
{
	auto src = create_stream(GetParam());
	std::stringstream dst;
	auto src_buf = std::make_shared<buffers::input_stream_buffer>(src);
	static constexpr std::size_t virtual_size = 10u;
	auto src_virtual_buf = std::make_shared<buffers::input_virtual_buffer>(
		src_buf, virtual_size);
	buffers::output_stream_buffer dst_buf(dst);
	buffers::input_buffer_stateful_wrapper wrapper(src_virtual_buf);
	ASSERT_EQ(buffers::copy(wrapper, dst_buf, GetParam() + virtual_size),
		GetParam() + virtual_size);
	ASSERT_EQ(dst.view().size(), src->view().size() + virtual_size);
	EXPECT_EQ(src->view(), dst.view().substr(0, dst.view().size() - virtual_size));
	EXPECT_EQ(dst.view().substr(dst.view().size() - virtual_size),
		std::string(virtual_size, '\0'));
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
	auto src_buf = std::make_shared<buffers::input_stream_buffer>(src);
	buffers::output_stream_buffer dst_buf(dst);
	buffers::input_buffer_stateful_wrapper wrapper(src_buf);
	EXPECT_THROW((void)buffers::copy(wrapper, dst_buf, src->view().size() + 100u),
		std::system_error);
}
