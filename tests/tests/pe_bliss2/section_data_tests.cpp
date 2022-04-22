#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <system_error>

#include "pe_bliss2/section_header.h"
#include "pe_bliss2/section_data.h"

#include "tests/tests/pe_bliss2/input_buffer_mock.h"

using namespace ::testing;

namespace
{

auto create_buffer_mock(std::size_t size, std::size_t initial_rpos,
	std::size_t absolute_offset, std::size_t relative_offset)
{
	auto rpos = std::make_shared<std::size_t>(initial_rpos);
	auto buffer_mock = std::make_shared<NiceMock<input_buffer_mock>>();
	ON_CALL(*buffer_mock, size)
		.WillByDefault(Return(size));
	ON_CALL(*buffer_mock, absolute_offset)
		.WillByDefault(Return(absolute_offset));
	ON_CALL(*buffer_mock, relative_offset)
		.WillByDefault(Return(relative_offset));
	ON_CALL(*buffer_mock, set_rpos)
		.WillByDefault([rpos](std::size_t value) {
			*rpos = value;
		});
	ON_CALL(*buffer_mock, advance_rpos)
		.WillByDefault([rpos](std::int32_t value) {
			*rpos += value;
		});
	ON_CALL(*buffer_mock, rpos)
		.WillByDefault([rpos]() {
			return *rpos;
		});
	ON_CALL(*buffer_mock, read)
		.WillByDefault([](std::size_t size, std::byte* data) {
			if (data) {
				std::memset(data, 0, size);
			}
			return size;
		});
	return buffer_mock;
}

static constexpr auto pointer_to_raw_data
= pe_bliss::section_header::max_raw_address_rounded_to_0 + 1u;
static constexpr std::size_t raw_size = 128u;
static constexpr std::size_t image_start_buffer_pos = 123u;
static constexpr std::size_t absolute_offset = 11u;
static constexpr std::size_t relative_offset = 15u;

auto create_section_header()
{
	pe_bliss::section_header header;
	header.set_pointer_to_raw_data(pointer_to_raw_data);
	header.set_raw_size(raw_size);
	return header;
}

void check_buffer(const pe_bliss::section_data& data)
{
	EXPECT_EQ(data.data()->size(), raw_size);
	EXPECT_EQ(data.data()->absolute_offset(), absolute_offset
		+ pointer_to_raw_data + image_start_buffer_pos);
	EXPECT_EQ(data.data()->relative_offset(), 0u);
	EXPECT_EQ(data.data()->rpos(), 0u);
}

} //namespace

TEST(SectionDataTests, DeserializeNoCopyTest)
{
	auto buffer_mock = create_buffer_mock(
		pointer_to_raw_data + raw_size + image_start_buffer_pos,
		55u, absolute_offset, relative_offset);
	EXPECT_CALL(*buffer_mock, set_rpos(_)).Times(0);
	EXPECT_CALL(*buffer_mock, advance_rpos(_)).Times(0);
	EXPECT_CALL(*buffer_mock, read(_, _)).Times(0);

	pe_bliss::section_data data;
	data.deserialize(create_section_header(), buffer_mock, {
		.section_alignment = 128u,
		.copy_memory = false,
		.image_loaded_to_memory = false,
		.image_start_buffer_pos = image_start_buffer_pos
	});

	check_buffer(data);
	EXPECT_EQ(buffer_mock.use_count(), 2);

	data.data()->set_rpos(10u);

	EXPECT_CALL(*buffer_mock, set_rpos(pointer_to_raw_data
		+ image_start_buffer_pos + data.data()->rpos()));
	EXPECT_CALL(*buffer_mock, read(1, nullptr));
	(void)data.data()->read(1, nullptr);
}

TEST(SectionDataTests, DeserializeCopyTest)
{
	auto buffer_mock = create_buffer_mock(
		pointer_to_raw_data + raw_size + image_start_buffer_pos,
		55u, absolute_offset, relative_offset);
	EXPECT_CALL(*buffer_mock, set_rpos(pointer_to_raw_data
		+ image_start_buffer_pos));
	EXPECT_CALL(*buffer_mock, advance_rpos(_)).Times(0);
	EXPECT_CALL(*buffer_mock, read(raw_size, _));

	pe_bliss::section_data data;
	data.deserialize(create_section_header(), buffer_mock, {
		.section_alignment = 128u,
		.copy_memory = true,
		.image_loaded_to_memory = false,
		.image_start_buffer_pos = image_start_buffer_pos
	});
	
	check_buffer(data);
	EXPECT_EQ(buffer_mock.use_count(), 1);
}

TEST(SectionDataTests, DeserializeLoadedTest)
{
	static constexpr std::uint32_t rva = 12345u;

	auto buffer_mock = create_buffer_mock(
		rva + raw_size + image_start_buffer_pos,
		55u, absolute_offset, relative_offset);
	EXPECT_CALL(*buffer_mock, set_rpos(rva + image_start_buffer_pos));
	EXPECT_CALL(*buffer_mock, advance_rpos(_)).Times(0);
	EXPECT_CALL(*buffer_mock, read(raw_size, _));

	auto header = create_section_header();
	header.set_rva(rva);
	pe_bliss::section_data data;
	data.deserialize(header, buffer_mock, {
		.section_alignment = 128u,
		.copy_memory = true,
		.image_loaded_to_memory = true,
		.image_start_buffer_pos = image_start_buffer_pos
	});

	EXPECT_EQ(buffer_mock.use_count(), 1);
}

TEST(SectionDataTests, DeserializeTooSmallTest)
{
	auto buffer_mock = create_buffer_mock(raw_size,
		55u, absolute_offset, relative_offset);

	pe_bliss::section_data data;
	EXPECT_THROW(data.deserialize(create_section_header(), buffer_mock, {
		.section_alignment = 128u,
		.copy_memory = true,
		.image_loaded_to_memory = false,
		.image_start_buffer_pos = image_start_buffer_pos
	}), std::system_error);
}

TEST(SectionDataTests, DeserializeIntOverflowTest)
{
	auto buffer_mock = create_buffer_mock(
		pointer_to_raw_data + raw_size + image_start_buffer_pos,
		55u, absolute_offset, relative_offset);

	auto header = create_section_header();
	pe_bliss::section_data data;
	EXPECT_THROW(data.deserialize(create_section_header(), buffer_mock, {
		.section_alignment = 128u,
		.copy_memory = true,
		.image_loaded_to_memory = false,
		.image_start_buffer_pos = (std::numeric_limits<std::size_t>::max)() - 3u
	}), std::system_error);
}
