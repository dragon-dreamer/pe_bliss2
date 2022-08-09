#include <cstddef>
#include <vector>

#include "gtest/gtest.h"

#include "buffers/input_container_buffer.h"
#include "tests/tests/buffers/input_buffer_helpers.h"

TEST(BufferTests, InputContainerBufferTest)
{
	const std::vector data{
		std::byte{1},
		std::byte{2},
		std::byte{3},
		std::byte{4},
		std::byte{5}
	};

	buffers::input_container_buffer buffer;
	buffer.get_container() = data;
	test_input_buffer(buffer, data);
	EXPECT_TRUE(buffer.is_stateless());
	EXPECT_EQ(buffer.virtual_size(), 0u);
}
