#include "pe_bliss2/resources/icon_cursor_reader.h"

#include <array>
#include <cstddef>

#include "gtest/gtest.h"

#include "pe_bliss2/resources/icon_cursor.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::resources;

TEST(IconCursorValidationTests, EmptyIcon)
{
	icon_group group;
	expect_contains_errors(validate_icon(group),
		icon_cursor_reader_errc::invalid_header);
}

TEST(IconCursorValidationTests, EmptyIconValid)
{
	icon_group group;
	group.get_header()->type = icon_group::type;
	expect_contains_errors(validate_icon(group));
}

TEST(IconCursorValidationTests, IconDifferentNumberOfHeadersAndData)
{
	icon_group group;
	group.get_header()->type = icon_group::type;
	group.get_data_list().resize(1);
	expect_contains_errors(validate_icon(group),
		icon_cursor_reader_errc::different_number_of_headers_and_data);
}

TEST(IconCursorValidationTests, EmptyCursor)
{
	cursor_group group;
	expect_contains_errors(validate_cursor(group),
		icon_cursor_reader_errc::invalid_header);
}

TEST(IconCursorValidationTests, EmptyCursorValid)
{
	cursor_group group;
	group.get_header()->type = cursor_group::type;
	expect_contains_errors(validate_cursor(group));
}

TEST(IconCursorValidationTests, CursorDifferentNumberOfHeadersAndData)
{
	cursor_group group;
	group.get_header()->type = cursor_group::type;
	group.get_data_list().resize(1);
	expect_contains_errors(validate_cursor(group),
		icon_cursor_reader_errc::different_number_of_headers_and_data);
}

TEST(IconCursorValidationTests, CursorZeroDimensions)
{
	cursor_group group;
	group.get_header()->type = cursor_group::type;
	group.get_data_list().resize(1);
	group.get_resource_group_headers().resize(1);
	expect_contains_errors(validate_cursor(group),
		icon_cursor_reader_errc::invalid_dimensions,
		icon_cursor_reader_errc::invalid_hotspot);
}

TEST(IconCursorValidationTests, CursorInvalidHotspot)
{
	cursor_group group;
	group.get_header()->type = cursor_group::type;
	group.get_data_list().resize(1);
	auto& header = group.get_resource_group_headers().emplace_back();
	header.native()->height = 16u;
	header.native()->width = 16u;
	expect_contains_errors(validate_cursor(group),
		icon_cursor_reader_errc::invalid_hotspot);
}

TEST(IconCursorValidationTests, CursorInvalidHotspot2)
{
	cursor_group group;
	group.get_header()->type = cursor_group::type;
	auto& hotspot = group.get_data_list().emplace_back();
	static constexpr std::array hotspot_data{
		std::byte{16}, std::byte{},
		std::byte{}, std::byte{}
	};
	hotspot.copied_data().assign(hotspot_data.begin(), hotspot_data.end());
	auto& header = group.get_resource_group_headers().emplace_back();
	header.native()->height = 16u;
	header.native()->width = 16u;
	expect_contains_errors(validate_cursor(group),
		icon_cursor_reader_errc::invalid_hotspot);
}

TEST(IconCursorValidationTests, CursorValid)
{
	cursor_group group;
	group.get_header()->type = cursor_group::type;
	auto& hotspot = group.get_data_list().emplace_back();
	static constexpr std::array hotspot_data{
		std::byte{15}, std::byte{},
		std::byte{}, std::byte{}
	};
	hotspot.copied_data().assign(hotspot_data.begin(), hotspot_data.end());
	auto& header = group.get_resource_group_headers().emplace_back();
	header.native()->height = 16u;
	header.native()->width = 16u;
	expect_contains_errors(validate_cursor(group));
}
