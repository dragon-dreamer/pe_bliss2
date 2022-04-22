#pragma once

#include <array>
#include <cstddef>
#include <system_error>

template<typename Buffer>
concept has_offset_setters = requires(Buffer buffer)
{
	{ buffer.set_absolute_offset({}) };
	{ buffer.set_relative_offset({}) };
};

template<typename Exception = std::system_error,
	typename Buffer, typename SourceData>
void test_input_buffer(Buffer& buffer, const SourceData& data,
	std::size_t initial_absolute_offset = 0u,
	std::size_t initial_relative_offset = 0u)
{
	static constexpr std::size_t test_buffer_size = 5u;
	EXPECT_EQ(buffer.size(), std::size(data));
	EXPECT_EQ(buffer.size(), test_buffer_size);
	EXPECT_EQ(buffer.rpos(), 0u);
	EXPECT_EQ(buffer.absolute_offset(), initial_absolute_offset);
	EXPECT_EQ(buffer.relative_offset(), initial_relative_offset);

	if constexpr (has_offset_setters<Buffer>)
	{
		buffer.set_absolute_offset(3u);
		buffer.set_relative_offset(4u);
		EXPECT_EQ(buffer.absolute_offset(), 3u);
		EXPECT_EQ(buffer.relative_offset(), 4u);
	}
	
	std::array<std::byte, 2u> copy{};
	EXPECT_EQ(buffer.read(copy.size(), copy.data()), copy.size());
	EXPECT_EQ(buffer.rpos(), 2u);
	EXPECT_EQ(copy[0], data[0]);
	EXPECT_EQ(copy[1], data[1]);

	std::array<std::byte, 3u> copy2{};
	EXPECT_EQ(buffer.read(10u, copy2.data()), copy2.size());
	EXPECT_EQ(buffer.rpos(), test_buffer_size);
	EXPECT_EQ(copy2[0], data[2]);
	EXPECT_EQ(copy2[1], data[3]);
	EXPECT_EQ(copy2[2], data[4]);

	EXPECT_EQ(buffer.read(10u, copy.data()), 0u);
	EXPECT_EQ(copy[0], data[0]);
	EXPECT_EQ(copy[1], data[1]);

	EXPECT_NO_THROW(buffer.set_rpos(test_buffer_size));
	EXPECT_EQ(buffer.rpos(), test_buffer_size);
	EXPECT_NO_THROW(buffer.set_rpos(1u));
	EXPECT_EQ(buffer.rpos(), 1u);
	EXPECT_THROW(buffer.set_rpos(test_buffer_size + 1u), Exception);
	EXPECT_EQ(buffer.rpos(), 1u);

	EXPECT_THROW(buffer.advance_rpos(-2), Exception);
	EXPECT_THROW(buffer.advance_rpos(test_buffer_size), Exception);
	EXPECT_NO_THROW(buffer.advance_rpos(-1));
	EXPECT_EQ(buffer.rpos(), 0u);
	EXPECT_NO_THROW(buffer.advance_rpos(3u));
	EXPECT_EQ(buffer.rpos(), 3u);
}
