#include <cstddef>
#include <memory>
#include <sstream>
#include <vector>

#include "gtest/gtest.h"

#include "buffers/input_stream_buffer.h"
#include "tests/tests/buffers/input_buffer_helpers.h"

TEST(BufferTests, InputStreamBufferTest)
{
	const std::vector<std::byte> data{
		std::byte(1),
		std::byte(2),
		std::byte(3),
		std::byte(4),
		std::byte(5)
	};

	auto ss = std::make_shared<std::stringstream>();
	for (auto b : data)
		*ss << static_cast<char>(b);

	buffers::input_stream_buffer buffer(ss);
	test_input_buffer<std::istream::failure>(buffer, data);
}
