#include "pe_bliss2/resources/icon_cursor_writer.h"

#include <array>
#include <cstddef>
#include <ranges>

#include "gtest/gtest.h"

#include "buffers/input_container_buffer.h"
#include "buffers/input_virtual_buffer.h"
#include "buffers/output_memory_buffer.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/detail/resources/icon_cursor.h"
#include "pe_bliss2/resources/icon_cursor.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::resources;

namespace
{
constexpr std::array data1{
	std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4},
	std::byte{5}
};

constexpr std::array data2{
	std::byte{4}, std::byte{5}, std::byte{6}, std::byte{7},
	std::byte{8}, std::byte{9}
};
} //namespace

TEST(IconCursorWriterTests, IconToFileFormat)
{
	icon_group group;
	group.get_header()->type = icon_group::type;
	auto& header1 = group.get_resource_group_headers().emplace_back();
	header1->height = 1u;
	auto& header2 = group.get_resource_group_headers().emplace_back();
	header1->height = 2u;
	
	auto& buf1 = group.get_data_list().emplace_back();
	buf1.copied_data().assign(data1.begin(), data1.end());
	auto& buf2 = group.get_data_list().emplace_back();
	buf2.copied_data().assign(data2.begin(), data2.end());

	auto file_group = to_file_format(group);
	EXPECT_EQ(file_group.get_header()->type, group.get_header()->type);

	ASSERT_EQ(file_group.get_data_list().size(), 2u);
	EXPECT_EQ(file_group.get_data_list()[0].data(), group.get_data_list()[0].data());
	EXPECT_EQ(file_group.get_data_list()[1].data(), group.get_data_list()[1].data());
	
	ASSERT_EQ(file_group.get_resource_group_headers().size(), 2u);
	EXPECT_EQ(file_group.get_resource_group_headers()[0]->height,
		group.get_resource_group_headers()[0]->height);
	EXPECT_EQ(file_group.get_resource_group_headers()[1]->height,
		group.get_resource_group_headers()[1]->height);

	EXPECT_EQ(file_group.get_resource_group_headers()[0]->size_in_bytes,
		data1.size());
	EXPECT_EQ(file_group.get_resource_group_headers()[1]->size_in_bytes,
		data2.size());

	static constexpr auto offset = file_icon_group::header_type::packed_size
		+ file_icon_group::resource_group_header_type::header_type::packed_size * 2u;
	EXPECT_EQ(file_group.get_resource_group_headers()[0]->image_offset,
		offset);
	EXPECT_EQ(file_group.get_resource_group_headers()[1]->image_offset,
		offset + data1.size());
}

TEST(IconCursorWriterTests, CursorToFileFormatInvalidHotspot)
{
	cursor_group group;
	group.get_header()->type = cursor_group::type;
	group.get_resource_group_headers().resize(2);
	group.get_data_list().resize(2);
	expect_throw_pe_error([&group] {
		(void)to_file_format(group);
	}, icon_cursor_writer_errc::invalid_hotspot);
}

TEST(IconCursorWriterTests, CursorToFileFormat)
{
	cursor_group group;
	group.get_header()->type = cursor_group::type;
	auto& header1 = group.get_resource_group_headers().emplace_back();
	header1->height = 2u;
	auto& header2 = group.get_resource_group_headers().emplace_back();
	header1->height = 4u;

	auto& buf1 = group.get_data_list().emplace_back();
	buf1.copied_data().assign(data1.begin(), data1.end());
	auto& buf2 = group.get_data_list().emplace_back();
	buf2.copied_data().assign(data2.begin(), data2.end());

	auto file_group = to_file_format(group);
	EXPECT_EQ(file_group.get_header()->type, group.get_header()->type);

	ASSERT_EQ(file_group.get_data_list().size(), 2u);
	static constexpr auto hotspot_size
		= pe_bliss::detail::packed_reflection::get_type_size<
		pe_bliss::detail::resources::cursor_hotspots>();
	EXPECT_TRUE(std::ranges::equal(file_group.get_data_list()[0].copied_data(),
		data1 | std::views::drop(hotspot_size)));
	EXPECT_TRUE(std::ranges::equal(file_group.get_data_list()[1].copied_data(),
		data2 | std::views::drop(hotspot_size)));

	ASSERT_EQ(file_group.get_resource_group_headers().size(), 2u);
	EXPECT_EQ(file_group.get_resource_group_headers()[0]->height,
		group.get_resource_group_headers()[0]->height / 2u);
	EXPECT_EQ(file_group.get_resource_group_headers()[1]->height,
		group.get_resource_group_headers()[1]->height / 2u);

	EXPECT_EQ(file_group.get_resource_group_headers()[0]->size_in_bytes,
		data1.size() - hotspot_size);
	EXPECT_EQ(file_group.get_resource_group_headers()[1]->size_in_bytes,
		data2.size() - hotspot_size);

	static constexpr auto offset = file_icon_group::header_type::packed_size
		+ file_icon_group::resource_group_header_type::header_type::packed_size * 2u;
	EXPECT_EQ(file_group.get_resource_group_headers()[0]->image_offset,
		offset);
	EXPECT_EQ(file_group.get_resource_group_headers()[1]->image_offset,
		offset + data1.size() - hotspot_size);

	EXPECT_EQ(file_group.get_resource_group_headers()[0]->hotspot_x, 0x0201u);
	EXPECT_EQ(file_group.get_resource_group_headers()[0]->hotspot_y, 0x0403u);
	EXPECT_EQ(file_group.get_resource_group_headers()[1]->hotspot_x, 0x0504u);
	EXPECT_EQ(file_group.get_resource_group_headers()[1]->hotspot_y, 0x0706u);
}

TEST(IconCursorWriterTests, CursorDifferentHeaderAndDataSize)
{
	cursor_group group;
	group.get_header()->type = cursor_group::type;
	group.get_resource_group_headers().resize(2);
	group.get_data_list().resize(1);
	expect_throw_pe_error([&group] {
		(void)to_file_format(group);
	}, icon_cursor_writer_errc::different_number_of_headers_and_data);
}

TEST(IconCursorWriterTests, IconVirtualData)
{
	icon_group group;
	group.get_header()->type = icon_group::type;
	group.get_resource_group_headers().emplace_back();

	auto& buf1 = group.get_data_list().emplace_back();
	auto buf = std::make_shared<buffers::input_container_buffer>();
	auto virtual_buf = std::make_shared<buffers::input_virtual_buffer>(std::move(buf), 5u);
	buf1.deserialize(virtual_buf, false);

	{
		auto file_group = to_file_format(group);
		EXPECT_EQ(file_group.get_resource_group_headers()[0]->size_in_bytes, 0u);
	}
	{
		auto file_group = to_file_format(group, { .write_virtual_part = true });
		EXPECT_EQ(file_group.get_resource_group_headers()[0]->size_in_bytes,
			virtual_buf->size());
	}
}

namespace
{
template<typename Group>
buffers::output_memory_buffer::buffer_type get_expected_serialized_data(
	const Group& group, bool write_virtual_data)
{
	buffers::output_memory_buffer::buffer_type data;
	buffers::output_memory_buffer buf(data);
	group.get_header().serialize(buf, write_virtual_data);
	for (const auto& entry_header : group.get_resource_group_headers())
		entry_header.native().serialize(buf, write_virtual_data);
	for (const auto& data : group.get_data_list())
		data.serialize(buf, write_virtual_data);
	return data;
}
} //namespace

TEST(IconCursorWriterTests, WriteIcon)
{
	file_icon_group group;
	group.get_header()->type = file_icon_group::type;

	group.get_resource_group_headers().emplace_back()->height = 10u;
	group.get_resource_group_headers().emplace_back()->height = 20u;
	group.get_data_list().emplace_back().copied_data()
		.assign(data1.begin(), data1.end());
	group.get_data_list().emplace_back().copied_data()
		.assign(data2.begin(), data2.end());

	buffers::output_memory_buffer::buffer_type data;
	buffers::output_memory_buffer buf(data);
	ASSERT_NO_THROW(write_icon(group, buf));
	EXPECT_EQ(data, get_expected_serialized_data(group, false));
}

TEST(IconCursorWriterTests, WriteIconVirtual)
{
	file_icon_group group;
	group.get_header()->type = file_icon_group::type;

	group.get_resource_group_headers().emplace_back()->height = 10u;
	group.get_resource_group_headers().emplace_back()->height = 20u;
	group.get_data_list().emplace_back().copied_data()
		.assign(data1.begin(), data1.end());
	auto buf2 = std::make_shared<buffers::input_container_buffer>();
	auto virtual_buf = std::make_shared<buffers::input_virtual_buffer>(std::move(buf2), 5u);
	group.get_data_list().emplace_back().deserialize(virtual_buf, false);

	buffers::output_memory_buffer::buffer_type data;
	buffers::output_memory_buffer buf(data);
	ASSERT_NO_THROW(write_icon(group, buf));
	EXPECT_EQ(data, get_expected_serialized_data(group, false));

	data.clear();
	buf.set_wpos(0);
	ASSERT_NO_THROW(write_icon(group, buf, { .write_virtual_part = true }));
	EXPECT_EQ(data, get_expected_serialized_data(group, true));
}
