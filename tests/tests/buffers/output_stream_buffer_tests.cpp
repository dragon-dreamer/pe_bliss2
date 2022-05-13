#include <array>
#include <cstddef>
#include <sstream>

#include "gtest/gtest.h"

#include "buffers/output_stream_buffer.h"
#include "tests/tests/buffers/output_buffer_helpers.h"

namespace
{
class stringstream_accessor
{
public:
	explicit stringstream_accessor(std::stringstream& ss) noexcept
		: ss_(ss)
	{
	}

	std::byte operator[](std::size_t index) const noexcept
	{
		return static_cast<std::byte>(ss_.view()[index]);
	}

	std::byte back() const noexcept
	{
		return static_cast<std::byte>(ss_.view().back());
	}

	std::size_t size() const noexcept
	{
		return ss_.view().size();
	}

private:
	std::stringstream& ss_;
};
} //namespace

TEST(BufferTests, OutputStreamBufferTest)
{
	static constexpr std::array data{
		std::byte{1},
		std::byte{2},
		std::byte{3},
		std::byte{4},
		std::byte{5}
	};

	std::stringstream ss;
	for (auto b : data)
		ss << static_cast<char>(b);

	ss.seekp(0, std::ios::beg);

	buffers::output_stream_buffer buffer(ss);

	test_output_buffer(buffer, stringstream_accessor(ss), false);
}
