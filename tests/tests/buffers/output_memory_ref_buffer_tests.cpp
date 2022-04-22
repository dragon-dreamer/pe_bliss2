#include <array>
#include <cstddef>
#include <system_error>

#include "gtest/gtest.h"

#include "buffers/output_memory_ref_buffer.h"

TEST(BufferTests, OutputMemoryRefBufferTest)
{
	std::array data{
		std::byte(1),
		std::byte(2),
		std::byte(3),
		std::byte(4),
		std::byte(5)
	};

	static constexpr std::array source{
		std::byte(10),
		std::byte(11)
	};

	buffers::output_memory_ref_buffer buffer(
		data.data(), data.size());

	EXPECT_EQ(buffer.size(), data.size());
	EXPECT_EQ(buffer.wpos(), 0u);

	std::size_t start_pos = 2u;
	EXPECT_NO_THROW(buffer.set_wpos(start_pos));
	EXPECT_EQ(buffer.wpos(), start_pos);

	//Overwrite 2 bytes inside the buffer
	EXPECT_NO_THROW(buffer.write(source.size(), source.data()));
	start_pos += source.size();
	EXPECT_EQ(buffer.wpos(), start_pos);
	EXPECT_EQ(buffer.size(), data.size());
	EXPECT_EQ(data[2], source[0]);
	EXPECT_EQ(data[3], source[1]);

	EXPECT_THROW(buffer.write(source.size(), source.data()),
		std::system_error);
	EXPECT_EQ(buffer.wpos(), start_pos);

	EXPECT_THROW(buffer.set_wpos(data.size() + 1u), std::system_error);
	EXPECT_EQ(buffer.wpos(), start_pos);

	EXPECT_NO_THROW(buffer.advance_wpos(1u));
	EXPECT_EQ(buffer.wpos(), start_pos + 1u);

	EXPECT_THROW(buffer.advance_wpos(1u), std::system_error);
	EXPECT_EQ(buffer.wpos(), start_pos + 1u);
}
