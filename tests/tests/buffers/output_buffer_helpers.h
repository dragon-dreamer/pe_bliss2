#pragma once

#include <cstddef>
#include <ios>

#include "gtest/gtest.h"

#include "buffers/output_buffer_interface.h"

template<typename SourceData>
void test_output_buffer(buffers::output_buffer_interface& buffer,
	const SourceData& data, bool test_expand = true)
{
	static constexpr std::array source{
		std::byte(10),
		std::byte(11)
	};

	auto initial_size = data.size();
	EXPECT_EQ(buffer.size(), initial_size);
	EXPECT_EQ(buffer.wpos(), 0u);

	std::size_t start_pos = 2u;
	EXPECT_NO_THROW(buffer.set_wpos(start_pos));
	EXPECT_EQ(buffer.wpos(), start_pos);

	//Overwrite 2 bytes inside the buffer
	EXPECT_NO_THROW(buffer.write(source.size(), source.data()));
	start_pos += source.size();
	EXPECT_EQ(buffer.wpos(), start_pos);
	EXPECT_EQ(buffer.size(), initial_size);
	EXPECT_EQ(data.size(), initial_size);
	EXPECT_EQ(data[2], source[0]);
	EXPECT_EQ(data[3], source[1]);

	//Expand buffer
	EXPECT_NO_THROW(buffer.write(source.size(), source.data()));
	start_pos += source.size();
	EXPECT_EQ(buffer.wpos(), start_pos);
	EXPECT_EQ(buffer.size(), initial_size + 1u);
	EXPECT_EQ(data.size(), initial_size + 1u);;
	EXPECT_EQ(data[4], source[0]);
	EXPECT_EQ(data[5], source[1]);

	static constexpr std::size_t new_size = 10u;
	if (test_expand)
	{
		EXPECT_NO_THROW(buffer.set_wpos(new_size));
		EXPECT_EQ(buffer.wpos(), new_size);
		EXPECT_EQ(buffer.size(), new_size);
		EXPECT_EQ(data.size(), new_size);
		EXPECT_EQ(data.back(), std::byte{});

		static constexpr std::size_t advanced_size = 5u;
		EXPECT_NO_THROW(buffer.advance_wpos(advanced_size));
		EXPECT_EQ(buffer.wpos(), new_size + advanced_size);
		EXPECT_EQ(buffer.size(), new_size + advanced_size);
		EXPECT_EQ(data.size(), new_size + advanced_size);
		EXPECT_EQ(data.back(), std::byte{});

		EXPECT_NO_THROW(buffer.advance_wpos(-2));
		EXPECT_EQ(buffer.wpos(), new_size + advanced_size - 2u);
	}
	else
	{
		EXPECT_THROW(buffer.set_wpos(new_size), std::ios::failure);
		EXPECT_NO_THROW(buffer.advance_wpos(-2));
		EXPECT_EQ(buffer.wpos(), start_pos - 2u);
		EXPECT_NO_THROW(buffer.advance_wpos(2u));
		EXPECT_EQ(buffer.wpos(), start_pos);
	}
}
