#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "buffers/input_memory_buffer.h"
#include "buffers/input_virtual_buffer.h"
#include "buffers/input_buffer_stateful_wrapper.h"
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
constexpr std::size_t extra_data_size = 3u;
} //namespace

TEST(BufferTests, InputVirtualBufferTest1)
{
	static constexpr std::array arr{
		std::byte{1},
		std::byte{2},
		std::byte{3},
		std::byte{4},
		std::byte{5}
	};

	auto buffer = std::make_shared<buffers::input_memory_buffer>(
		arr.data(), arr.size());
	buffers::input_virtual_buffer virtual_buffer(buffer, 0u);
	test_input_buffer(virtual_buffer, arr);
	EXPECT_TRUE(virtual_buffer.is_stateless());
	EXPECT_EQ(virtual_buffer.virtual_size(), 0u);
}

TEST(BufferTests, InputVirtualBufferTest2)
{
	auto buffer = std::make_shared<buffers::input_memory_buffer>(
		data.data(), data.size());
	buffers::input_virtual_buffer virtual_buffer(buffer, extra_data_size);
	EXPECT_TRUE(virtual_buffer.is_stateless());
	EXPECT_EQ(virtual_buffer.virtual_size(), extra_data_size);

	EXPECT_EQ(virtual_buffer.size(), data.size() + extra_data_size);
	std::vector<std::byte> vec(data.size(), std::byte{ 1 });
	EXPECT_EQ(virtual_buffer.read(1u, data.size(), vec.data()),
		data.size() - 1u);
	EXPECT_EQ(vec, (std::vector<std::byte>{
		std::byte{ 2 },
		std::byte{ 3 },
		std::byte{ 4 },
		std::byte{ 5 },
		std::byte{ 0 }}));

	EXPECT_EQ(virtual_buffer.read(6u, extra_data_size - 1u, vec.data()), 0u);
	EXPECT_EQ(vec, (std::vector<std::byte>{
		std::byte{ 0 },
		std::byte{ 0 },
		std::byte{ 4 },
		std::byte{ 5 },
		std::byte{ 0 }}));

	std::byte byte{ 1 };
	EXPECT_EQ(virtual_buffer.read(8u, 0u, &byte), 0u);
	EXPECT_EQ(byte, std::byte{ 1 });

	EXPECT_EQ(virtual_buffer.read(7u, 1u, &byte), 0u);
	EXPECT_EQ(byte, std::byte{ 0 });
}

TEST(BufferTests, InputVirtualBufferNestedTest)
{
	auto buffer = std::make_shared<buffers::input_memory_buffer>(
		data.data(), data.size());
	auto virtual_buffer = std::make_shared<buffers::input_virtual_buffer>(
		buffer, extra_data_size);
	static constexpr std::size_t extra_data_size_top = 2u;
	buffers::input_virtual_buffer virtual_buffer_top(virtual_buffer, extra_data_size_top);

	ASSERT_EQ(virtual_buffer_top.size(), buffer->size()
		+ extra_data_size + extra_data_size_top);
	EXPECT_EQ(virtual_buffer_top.virtual_size(), extra_data_size_top + extra_data_size);
	EXPECT_TRUE(virtual_buffer_top.is_stateless());

	std::vector<std::byte> vec(virtual_buffer_top.size(), std::byte{ 1 });
	EXPECT_EQ(virtual_buffer_top.read(0u, virtual_buffer_top.size(), vec.data()),
		buffer->size());
	EXPECT_TRUE(std::equal(data.begin(), data.end(), vec.begin()));
	EXPECT_TRUE(std::all_of(vec.begin() + data.size(), vec.end(),
		[](auto val) { return val == std::byte{}; }));
}
