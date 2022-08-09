#pragma once

#include <array>
#include <cstddef>
#include <system_error>

#include "buffers/input_buffer_stateful_wrapper.h"

template<typename Buffer>
concept has_offset_setters = requires(Buffer buffer)
{
	{ buffer.set_absolute_offset({}) };
	{ buffer.set_relative_offset({}) };
};

template<typename Buffer, typename SourceData>
void test_input_buffer(Buffer& buffer, const SourceData& data,
	std::size_t initial_absolute_offset = 0u,
	std::size_t initial_relative_offset = 0u)
{
	static constexpr std::size_t test_buffer_size = 5u;
	buffers::input_buffer_stateful_wrapper_ref wrapper(buffer);
	EXPECT_EQ(wrapper.size(), std::size(data));
	EXPECT_EQ(wrapper.size(), test_buffer_size);
	EXPECT_EQ(wrapper.rpos(), 0u);
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
	ASSERT_EQ(wrapper.read(copy.size(), copy.data()), copy.size());
	EXPECT_EQ(wrapper.rpos(), 2u);
	EXPECT_EQ(copy[0], data[0]);
	EXPECT_EQ(copy[1], data[1]);

	std::array<std::byte, 3u> copy2{};
	ASSERT_EQ(wrapper.read(copy2.size(), copy2.data()), copy2.size());
	EXPECT_EQ(wrapper.rpos(), test_buffer_size);
	EXPECT_EQ(copy2[0], data[2]);
	EXPECT_EQ(copy2[1], data[3]);
	EXPECT_EQ(copy2[2], data[4]);

	ASSERT_THROW(wrapper.read(10u, copy.data()), std::system_error);
	EXPECT_EQ(copy[0], data[0]);
	EXPECT_EQ(copy[1], data[1]);

	ASSERT_NO_THROW(wrapper.set_rpos(test_buffer_size));
	EXPECT_EQ(wrapper.rpos(), test_buffer_size);
	ASSERT_NO_THROW(wrapper.set_rpos(1u));
	EXPECT_EQ(wrapper.rpos(), 1u);
	EXPECT_THROW(wrapper.set_rpos(test_buffer_size + 1u), std::system_error);
	EXPECT_EQ(wrapper.rpos(), 1u);

	EXPECT_THROW(wrapper.advance_rpos(-2), std::system_error);
	EXPECT_THROW(wrapper.advance_rpos(test_buffer_size), std::system_error);
	ASSERT_NO_THROW(wrapper.advance_rpos(-1));
	EXPECT_EQ(wrapper.rpos(), 0u);
	ASSERT_NO_THROW(wrapper.advance_rpos(3u));
	EXPECT_EQ(wrapper.rpos(), 3u);
}
