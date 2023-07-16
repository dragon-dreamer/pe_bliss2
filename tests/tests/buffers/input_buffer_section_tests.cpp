#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <span>
#include <system_error>
#include <vector>

#include "gtest/gtest.h"

#include "buffers/input_buffer_section.h"
#include "buffers/input_memory_buffer.h"
#include "buffers/input_virtual_buffer.h"
#include "tests/buffers/input_buffer_helpers.h"

namespace
{
constexpr std::array data{
	std::byte{1},
	std::byte{2},
	std::byte{3},
	std::byte{4},
	std::byte{5},
	std::byte{6},
	std::byte{7},
	std::byte{8}
};
} //namespace

TEST(BufferTests, InputBufferSectionTest)
{
	auto buffer = std::make_shared<buffers::input_memory_buffer>(
		data.data(), data.size());

	static constexpr std::size_t absolute_offset = 10u;
	static constexpr std::size_t relative_offset = 20u;
	buffer->set_absolute_offset(absolute_offset);
	buffer->set_relative_offset(relative_offset);

	std::size_t offset = 1u;
	static constexpr std::size_t size = 5u;
	buffers::input_buffer_section section(buffer, offset, size);
	test_input_buffer(section,
		std::span(data.begin() + offset, data.begin() + offset + size),
		absolute_offset + offset, relative_offset + offset);

	++offset;
	auto section2 = buffers::reduce(buffer, offset, size);
	test_input_buffer(*section2,
		std::span(data.begin() + offset, data.begin() + offset + size),
		absolute_offset + offset, relative_offset + offset);
}

TEST(BufferTests, InputBufferSectionConstructorTest)
{
	const std::vector<std::byte> vec{
		std::byte(1)
	};

	auto buffer = std::make_shared<buffers::input_memory_buffer>(
		vec.data(), vec.size());

	EXPECT_NO_THROW((void)buffers::input_buffer_section(buffer, 0u, 1u));
	EXPECT_NO_THROW((void)buffers::input_buffer_section(buffer, 1u, 0u));
	EXPECT_THROW((void)buffers::input_buffer_section(buffer, 1u, 1u), std::system_error);
	EXPECT_THROW((void)buffers::input_buffer_section(buffer,
		(std::numeric_limits<std::size_t>::max)(), 1u), std::system_error);
}

TEST(BufferTests, InputBufferSectionReduceTest)
{
	auto buffer = std::make_shared<buffers::input_memory_buffer>(
		data.data(), data.size());

	std::size_t offset = 1u;
	static constexpr std::size_t size = 7u;
	buffers::input_buffer_section section(buffer, offset, size);
	EXPECT_EQ(section.virtual_size(), 0u);
	EXPECT_TRUE(section.is_stateless());

	EXPECT_THROW((void)section.reduce(100u, 0u), std::system_error);
	EXPECT_THROW((void)section.reduce(1u, 100u), std::system_error);
	EXPECT_THROW((void)section.reduce((std::numeric_limits<std::size_t>::max)(),
		100u), std::system_error);
	EXPECT_THROW((void)section.reduce(100u,
		(std::numeric_limits<std::size_t>::max)()), std::system_error);

	static constexpr std::size_t reduced_offset = 1u;
	static constexpr std::size_t reduced_size = 5u;
	auto reduced_section = section.reduce(reduced_offset, reduced_size);
	test_input_buffer(reduced_section,
		std::span(data.begin() + reduced_offset + offset,
			data.begin() + reduced_offset + offset + reduced_size),
		reduced_offset + offset, reduced_offset + offset);
	EXPECT_TRUE(reduced_section.is_stateless());
}

TEST(BufferTests, InputBufferSectionVirtualTest)
{
	auto buffer = std::make_shared<buffers::input_memory_buffer>(
		data.data(), data.size());
	static constexpr std::size_t extra_virtual_size = 5u;
	auto virtual_buf = std::make_shared<buffers::input_virtual_buffer>(buffer,
		extra_virtual_size);

	buffers::input_buffer_section section(virtual_buf, 3u, 7u);
	EXPECT_EQ(section.size(), 7u);
	EXPECT_EQ(section.virtual_size(), 2u);
	std::vector<std::byte> vec(section.size(), std::byte{ 1 });
	EXPECT_EQ(section.read(0u, section.size(), vec.data()), 
		section.size() - section.virtual_size());
	EXPECT_EQ(vec, (std::vector<std::byte>{
		std::byte{ 4 },
		std::byte{ 5 },
		std::byte{ 6 },
		std::byte{ 7 },
		std::byte{ 8 },
		std::byte{},
		std::byte{}
	}));
	EXPECT_TRUE(section.is_stateless());
}

TEST(BufferTests, InputBufferSectionEmptyTest)
{
	auto buffer = std::make_shared<buffers::input_memory_buffer>(
		data.data(), data.size());
	static constexpr std::size_t extra_virtual_size = 1000u;
	auto virtual_buf = std::make_shared<buffers::input_virtual_buffer>(buffer,
		extra_virtual_size);

	buffers::input_buffer_section section(virtual_buf, 500u, 0u);
	EXPECT_EQ(section.size(), 0u);
	EXPECT_EQ(section.virtual_size(), 0u);
	EXPECT_EQ(section.physical_size(), 0u);
}

TEST(BufferTests, InputBufferGetRawDataTest)
{
	auto buffer = std::make_shared<buffers::input_memory_buffer>(
		data.data(), data.size());

	buffers::input_buffer_section section(buffer, 1u, 3u);
	const std::byte* ptr{};
	ASSERT_NO_THROW((ptr = section.get_raw_data(0u, 2u)));
	ASSERT_NE(ptr, nullptr);
	EXPECT_EQ(ptr[0], data[1]);
	EXPECT_EQ(ptr[1], data[2]);

	ASSERT_NO_THROW((ptr = section.get_raw_data(1u, 2u)));
	ASSERT_NE(ptr, nullptr);
	EXPECT_EQ(ptr[0], data[2]);

	EXPECT_THROW((ptr = section.get_raw_data(1, 3u)), std::system_error);
}
