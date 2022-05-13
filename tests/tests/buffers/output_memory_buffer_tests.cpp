#include <array>
#include <cstddef>
#include <vector>

#include "gtest/gtest.h"

#include "buffers/output_memory_buffer.h"
#include "tests/tests/buffers/output_buffer_helpers.h"

TEST(BufferTests, OutputMemoryBufferTest)
{
	std::vector data{
		std::byte{1},
		std::byte{2},
		std::byte{3},
		std::byte{4},
		std::byte{5}
	};

	buffers::output_memory_buffer buffer(data);

	test_output_buffer(buffer, data);
}
