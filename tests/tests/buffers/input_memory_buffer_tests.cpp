#include <array>
#include <cstddef>
#include <system_error>

#include "gtest/gtest.h"

#include "buffers/input_memory_buffer.h"
#include "tests/buffers/input_buffer_helpers.h"

namespace
{
constexpr std::array data{
	std::byte{1},
	std::byte{2},
	std::byte{3},
	std::byte{4},
	std::byte{5}
};
} //namespace

TEST(BufferTests, InputMemoryBufferTest)
{
	buffers::input_memory_buffer buffer(data.data(), data.size());
	test_input_buffer(buffer, data);
	EXPECT_TRUE(buffer.is_stateless());
	EXPECT_EQ(buffer.virtual_size(), 0u);
}

TEST(BufferTests, InputMemoryBufferGetRawDataTest)
{
	buffers::input_memory_buffer buffer(data.data(), data.size());

	const std::byte* ptr{};
	ASSERT_NO_THROW((ptr = buffer.get_raw_data(1u, 2u)));
	ASSERT_NE(ptr, nullptr);
	EXPECT_EQ(ptr[0], data[1]);
	EXPECT_EQ(ptr[1], data[2]);

	EXPECT_THROW((ptr = buffer.get_raw_data(1u, 5u)), std::system_error);
}
