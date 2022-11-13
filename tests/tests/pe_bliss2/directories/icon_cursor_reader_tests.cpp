#include "pe_bliss2/resources/icon_cursor_reader.h"

#include <array>
#include <cstddef>
#include <ranges>

#include "gtest/gtest.h"

#include "buffers/input_container_buffer.h"
#include "buffers/input_virtual_buffer.h"
#include "pe_bliss2/resources/resource_directory.h"
#include "pe_bliss2/resources/resource_writer.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::resources;

namespace
{

constexpr std::array data2{
	std::byte{1}, std::byte{2}, std::byte{3}
};

constexpr std::array data3{
	std::byte{4}, std::byte{5}, std::byte{6}, std::byte{7}
};

constexpr std::array icon_group_data{
	std::byte{}, std::byte{}, //reserved
	std::byte{1}, std::byte{}, //type
	std::byte{2}, std::byte{}, //count

	std::byte{16}, //width
	std::byte{16}, //height
	std::byte{16}, //color_count
	std::byte{}, //reserved
	std::byte{}, std::byte{}, //planes
	std::byte{8}, std::byte{}, //bit_count
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //size_in_bytes
	std::byte{2}, std::byte{}, //number

	std::byte{24}, //width
	std::byte{24}, //height
	std::byte{8}, //color_count
	std::byte{}, //reserved
	std::byte{}, std::byte{}, //planes
	std::byte{16}, std::byte{}, //bit_count
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //size_in_bytes
	std::byte{3}, std::byte{} //number
};

constexpr std::array cursor_group_data{
	std::byte{}, std::byte{}, //reserved
	std::byte{2}, std::byte{}, //type
	std::byte{2}, std::byte{}, //count
	
	std::byte{16}, std::byte{}, //width
	std::byte{32}, std::byte{}, //height
	std::byte{}, std::byte{}, //planes
	std::byte{8}, std::byte{}, //bit_count
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //size_in_bytes
	std::byte{2}, std::byte{}, //number

	std::byte{32}, std::byte{}, //width
	std::byte{64}, std::byte{}, //height
	std::byte{}, std::byte{}, //planes
	std::byte{16}, std::byte{}, //bit_count
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //size_in_bytes
	std::byte{3}, std::byte{}, //number
};

template<typename Group>
void check_group(Group group, bool copied, bool is_virtual = false)
{
	EXPECT_EQ(group.get_header()->type, Group::type);

	ASSERT_EQ(group.get_resource_group_headers().size(), 2u);
	EXPECT_EQ(group.get_resource_group_headers()[0]->number, 2u);
	EXPECT_EQ(group.get_resource_group_headers()[1]->number, 3u);

	ASSERT_EQ(group.get_data_list().size(), 2u);
	EXPECT_EQ(group.get_data_list()[0].is_copied(), copied);
	EXPECT_EQ(group.get_data_list()[1].is_copied(), copied);
	EXPECT_TRUE(std::ranges::equal(group.get_data_list()[0].copied_data(), data2));
	EXPECT_TRUE(std::ranges::equal(group.get_data_list()[1].copied_data(), data3));

	EXPECT_EQ(!!group.get_data_list()[1].virtual_size(), is_virtual);
}

} //namespace

TEST(IconCursorReader, ReadIcon)
{
	resource_directory root;
	auto& icon2 = try_emplace_resource_data_by_id(root, resource_type::icon, 2u, 1u);
	icon2.get_raw_data().copied_data().assign(data2.begin(), data2.end());

	auto& icon3 = try_emplace_resource_data_by_id(root, resource_type::icon, 3u, 1u);
	icon3.get_raw_data().copied_data().assign(data3.begin(), data3.end());

	auto& group = try_emplace_resource_data_by_id(root, resource_type::icon_group, 123u, 1u);
	group.get_raw_data().copied_data().assign(icon_group_data.begin(), icon_group_data.end());

	auto& named_group = try_emplace_resource_data_by_name(root, resource_type::icon_group, u"name", 1u);
	named_group.get_raw_data().copied_data().assign(icon_group_data.begin(), icon_group_data.end());
	
	//Invalid group for lang 2 and the same ID
	try_emplace_resource_data_by_id(root, resource_type::icon_group, 123u, 2u);

	check_group(icon_group_from_resource(root, 0u, 123u), false);
	check_group(icon_group_from_resource(root, 0u, 123u,
		{ .copy_memory = true }), true);

	check_group(icon_group_from_resource_by_lang(root, 1u, 123u), false);
	check_group(icon_group_from_resource(root, 0u, u"name"), false);
	check_group(icon_group_from_resource_by_lang(root, 1u, u"name"), false);

	auto buf = std::make_shared<buffers::input_container_buffer>();
	buf->get_container().assign(data3.begin(), data3.end());
	auto virtual_buf = std::make_shared<buffers::input_virtual_buffer>(std::move(buf), 1u);
	icon3.get_raw_data().deserialize(virtual_buf, false);
	expect_throw_pe_error([&root] {
		(void)icon_group_from_resource(root, 0u, 123u);
	}, icon_cursor_reader_errc::unable_to_read_data);

	check_group(icon_group_from_resource(root, 0u, 123u, {
		.allow_virtual_data = true }), false, true);
}

TEST(IconCursorReader, ReadCursor)
{
	resource_directory root;
	auto& cursor2 = try_emplace_resource_data_by_id(root, resource_type::cursor, 2u, 1u);
	cursor2.get_raw_data().copied_data().assign(data2.begin(), data2.end());

	auto& cursor3 = try_emplace_resource_data_by_id(root, resource_type::cursor, 3u, 1u);
	cursor3.get_raw_data().copied_data().assign(data3.begin(), data3.end());

	auto& group = try_emplace_resource_data_by_id(root, resource_type::cursor_group, 123u, 1u);
	group.get_raw_data().copied_data().assign(cursor_group_data.begin(), cursor_group_data.end());

	auto& named_group = try_emplace_resource_data_by_name(root, resource_type::cursor_group, u"name", 1u);
	named_group.get_raw_data().copied_data().assign(cursor_group_data.begin(), cursor_group_data.end());

	//Invalid group for lang 2 and the same ID
	try_emplace_resource_data_by_id(root, resource_type::cursor_group, 123u, 2u);

	check_group(cursor_group_from_resource(root, 0u, 123u), false);
	check_group(cursor_group_from_resource(root, 0u, 123u,
		{ .copy_memory = true }), true);

	check_group(cursor_group_from_resource_by_lang(root, 1u, 123u), false);
	check_group(cursor_group_from_resource(root, 0u, u"name"), false);
	check_group(cursor_group_from_resource_by_lang(root, 1u, u"name"), false);

	auto buf = std::make_shared<buffers::input_container_buffer>();
	buf->get_container().assign(data3.begin(), data3.end());
	auto virtual_buf = std::make_shared<buffers::input_virtual_buffer>(std::move(buf), 1u);
	cursor3.get_raw_data().deserialize(virtual_buf, false);
	expect_throw_pe_error([&root] {
		(void)cursor_group_from_resource(root, 0u, 123u);
	}, icon_cursor_reader_errc::unable_to_read_data);

	check_group(cursor_group_from_resource(root, 0u, 123u, {
		.allow_virtual_data = true }), false, true);
}
