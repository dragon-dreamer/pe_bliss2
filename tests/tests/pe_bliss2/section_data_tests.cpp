#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <system_error>

#include "buffers/input_buffer_stateful_wrapper.h"

#include "pe_bliss2/section/section_errc.h"
#include "pe_bliss2/section/section_header.h"
#include "pe_bliss2/section/section_data.h"
#include "pe_bliss2/section/section_table.h"

#include "tests/tests/pe_bliss2/input_buffer_mock.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace ::testing;
using namespace pe_bliss::section;

namespace
{

auto create_buffer_mock(std::size_t size, std::size_t initial_rpos,
	std::size_t absolute_offset, std::size_t relative_offset,
	std::size_t virtual_size = 0u)
{
	auto rpos = std::make_shared<std::size_t>(initial_rpos);
	auto buffer_mock = std::make_shared<NiceMock<input_buffer_mock>>();
	ON_CALL(*buffer_mock, size)
		.WillByDefault(Return(size + virtual_size));
	ON_CALL(*buffer_mock, virtual_size)
		.WillByDefault(Return(virtual_size));
	ON_CALL(*buffer_mock, is_stateless)
		.WillByDefault(Return(true));
	buffer_mock->set_absolute_offset(absolute_offset);
	buffer_mock->set_relative_offset(relative_offset);
	ON_CALL(*buffer_mock, read)
		.WillByDefault([](std::size_t /* pos */, std::size_t size, std::byte* data) {
			if (data) {
				std::memset(data, 0, size);
			}
			return size;
		});
	return buffer_mock;
}

static constexpr auto pointer_to_raw_data
	= pe_bliss::section::section_header::max_raw_address_rounded_to_0 + 1u;
static constexpr std::size_t raw_size = 128u;
static constexpr std::size_t image_start_buffer_pos = 123u;
static constexpr std::size_t absolute_offset = 11u;
static constexpr std::size_t relative_offset = 15u;

auto create_section_header()
{
	section_header header;
	header.set_pointer_to_raw_data(pointer_to_raw_data);
	header.set_raw_size(raw_size);
	return header;
}

void check_buffer(const section_data& data, std::size_t virtual_size = 0u)
{
	EXPECT_EQ(data.data()->size(), raw_size + virtual_size);
	EXPECT_EQ(data.data()->physical_size(), raw_size);
	EXPECT_EQ(data.data()->virtual_size(), virtual_size);
	EXPECT_TRUE(data.data()->is_stateless());
	EXPECT_EQ(data.data()->absolute_offset(), absolute_offset
		+ pointer_to_raw_data + image_start_buffer_pos);
	EXPECT_EQ(data.data()->relative_offset(), 0u);
}

void deserialize_no_copy_test(std::uint32_t extra_virtual_size)
{
	auto buffer_mock = create_buffer_mock(
		pointer_to_raw_data + raw_size + image_start_buffer_pos,
		55u, absolute_offset, relative_offset);
	EXPECT_CALL(*buffer_mock, read(_, _, _)).Times(0);

	section_data data;
	auto header = create_section_header();
	header.set_virtual_size(raw_size + extra_virtual_size);
	{
		buffers::input_buffer_stateful_wrapper wrapper(buffer_mock);

		data.deserialize(header, wrapper, {
			.section_alignment = 128u,
			.copy_memory = false,
			.image_loaded_to_memory = false,
			.image_start_buffer_pos = image_start_buffer_pos
		});

		check_buffer(data, extra_virtual_size);
	}
	EXPECT_EQ(buffer_mock.use_count(), 2);

	EXPECT_CALL(*buffer_mock, read(
		pointer_to_raw_data + image_start_buffer_pos + 10u, 1u, nullptr));
	(void)data.data()->read(10u, 1u, nullptr);
}

void deserialize_copy_test(std::uint32_t extra_virtual_size)
{
	auto buffer_mock = create_buffer_mock(
		pointer_to_raw_data + raw_size + image_start_buffer_pos,
		55u, absolute_offset, relative_offset);
	EXPECT_CALL(*buffer_mock, read(pointer_to_raw_data
		+ image_start_buffer_pos, raw_size, _));

	section_data data;
	auto header = create_section_header();
	header.set_virtual_size(raw_size + extra_virtual_size);

	{
		buffers::input_buffer_stateful_wrapper wrapper(buffer_mock);
		data.deserialize(header, wrapper, {
			.section_alignment = 128u,
			.copy_memory = true,
			.image_loaded_to_memory = false,
			.image_start_buffer_pos = image_start_buffer_pos
		});
	}

	check_buffer(data, extra_virtual_size);
	EXPECT_EQ(buffer_mock.use_count(), 1);
}

} //namespace

TEST(SectionDataTests, DeserializeNoCopyTest)
{
	deserialize_no_copy_test(0u);
}

TEST(SectionDataTests, DeserializeNoCopyVirtualTest)
{
	deserialize_no_copy_test(256u);
}

TEST(SectionDataTests, DeserializeCopyTest)
{
	deserialize_copy_test(0u);
}

TEST(SectionDataTests, DeserializeCopyVirtualTest)
{
	deserialize_copy_test(512u);
}

TEST(SectionDataTests, DeserializeLoadedTest)
{
	static constexpr std::uint32_t rva = 12345u;

	auto buffer_mock = create_buffer_mock(
		rva + raw_size + image_start_buffer_pos,
		55u, absolute_offset, relative_offset);
	EXPECT_CALL(*buffer_mock, read(
		rva + image_start_buffer_pos, raw_size, _));

	auto header = create_section_header();
	header.set_rva(rva);
	section_data data;

	{
		buffers::input_buffer_stateful_wrapper wrapper(buffer_mock);
		data.deserialize(header, wrapper, {
			.section_alignment = 128u,
			.copy_memory = true,
			.image_loaded_to_memory = true,
			.image_start_buffer_pos = image_start_buffer_pos
		});
	}

	EXPECT_EQ(buffer_mock.use_count(), 1);
}

TEST(SectionDataTests, DeserializeTooSmallTest)
{
	auto buffer_mock = create_buffer_mock(raw_size,
		55u, absolute_offset, relative_offset);

	buffers::input_buffer_stateful_wrapper wrapper(buffer_mock);
	section_data data;
	expect_throw_pe_error([&]() {
		data.deserialize(create_section_header(), wrapper, {
		.section_alignment = 128u,
		.copy_memory = true,
		.image_loaded_to_memory = false,
		.image_start_buffer_pos = image_start_buffer_pos
		});
	}, section_errc::unable_to_read_section_data);
}

TEST(SectionDataTests, DeserializeIntOverflowTest)
{
	auto buffer_mock = create_buffer_mock(
		pointer_to_raw_data + raw_size + image_start_buffer_pos,
		55u, absolute_offset, relative_offset);

	buffers::input_buffer_stateful_wrapper wrapper(buffer_mock);
	auto header = create_section_header();
	section_data data;
	expect_throw_pe_error([&]() {
		data.deserialize(create_section_header(), wrapper, {
		.section_alignment = 128u,
		.copy_memory = true,
		.image_loaded_to_memory = false,
		.image_start_buffer_pos = (std::numeric_limits<std::size_t>::max)() - 3u
		});
	}, section_errc::unable_to_read_section_data);
}
