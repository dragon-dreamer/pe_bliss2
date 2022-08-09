#include <cstddef>
#include <memory>
#include <sstream>
#include <vector>

#include "gtest/gtest.h"

#include "buffers/input_stream_buffer.h"
#include "buffers/input_virtual_buffer.h"
#include "buffers/input_buffer_section.h"
#include "tests/tests/buffers/input_buffer_helpers.h"

namespace
{
const std::vector data{
	std::byte{1},
	std::byte{2},
	std::byte{3},
	std::byte{4},
	std::byte{5}
};

std::shared_ptr<std::stringstream> create_stream()
{
	auto ss = std::make_shared<std::stringstream>();
	for (auto b : data)
		*ss << static_cast<char>(b);

	return ss;
}
} //namespace

TEST(BufferTests, InputStreamBufferTest)
{
	buffers::input_stream_buffer buffer(create_stream());
	test_input_buffer(buffer, data);
	EXPECT_FALSE(buffer.is_stateless());
	EXPECT_EQ(buffer.virtual_size(), 0u);
}

TEST(BufferTests, InputVirtualStreamBufferTest)
{
	auto buffer = std::make_shared<buffers::input_stream_buffer>(create_stream());
	buffers::input_virtual_buffer virtual_buffer(buffer, 0u);
	EXPECT_FALSE(virtual_buffer.is_stateless());
	EXPECT_EQ(virtual_buffer.virtual_size(), 0u);
}

TEST(BufferTests, InputStreamSectionBufferTest)
{
	auto buffer = std::make_shared<buffers::input_stream_buffer>(create_stream());
	auto section = buffers::reduce(buffer, 1u);
	EXPECT_FALSE(section->is_stateless());
	EXPECT_EQ(section->virtual_size(), 0u);
}
