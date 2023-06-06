#include <array>
#include <cstddef>

#include "gtest/gtest.h"

#include "buffers/input_memory_buffer.h"
#include "tests/buffers/input_buffer_helpers.h"

TEST(BufferTests, InputMemoryBufferTest)
{
	static constexpr std::array data{
		std::byte{1},
		std::byte{2},
		std::byte{3},
		std::byte{4},
		std::byte{5}
	};

	buffers::input_memory_buffer buffer(data.data(), data.size());
	test_input_buffer(buffer, data);
	EXPECT_TRUE(buffer.is_stateless());
	EXPECT_EQ(buffer.virtual_size(), 0u);
}
