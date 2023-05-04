#include "gtest/gtest.h"

#include <limits>
#include <string_view>

#include "pe_bliss2/rich/compid_database.h"

using namespace pe_bliss::rich;

TEST(CompidDatabaseTests, ProductTypeToNameTest)
{
	EXPECT_EQ(std::string_view(compid_database::product_type_to_string(
		compid_database::product_type::visual_studio_1997_sp3)),
		"Visual Studio 1997 SP3");
	EXPECT_EQ(std::string_view(compid_database::product_type_to_string(
		compid_database::product_type::visual_studio_2022_17_2_0_preview2_1)),
		"Visual Studio 2022 17.2.0 Preview 2.1");
	EXPECT_EQ(std::string_view(compid_database::product_type_to_string(
		compid_database::product_type::unknown)), "Unknown");
	EXPECT_EQ(std::string_view(compid_database::product_type_to_string(
		static_cast<compid_database::product_type>(-1))), "Unknown");
}

TEST(CompidDatabaseTests, ToolTypeToNameTest)
{
	EXPECT_EQ(std::string_view(compid_database::tool_type_to_string(
		compid_database::tool_type::assembly)), "Assembly");
	EXPECT_EQ(std::string_view(compid_database::tool_type_to_string(
		compid_database::tool_type::cpp_source)), "CPP Source");
	EXPECT_EQ(std::string_view(compid_database::tool_type_to_string(
		compid_database::tool_type::unknown)), "Unknown");
	EXPECT_EQ(std::string_view(compid_database::tool_type_to_string(
		static_cast<compid_database::tool_type>(-1))), "Unknown");
}

TEST(CompidDatabaseTests, GetToolTest)
{
	EXPECT_EQ(compid_database::get_tool(0x105),
		compid_database::tool_type::cpp_source);
	EXPECT_EQ(compid_database::get_tool(0),
		compid_database::tool_type::unknown);
	EXPECT_EQ(compid_database::get_tool(
		(std::numeric_limits<std::uint16_t>::max)()),
		compid_database::tool_type::unknown);
}

TEST(CompidDatabaseTests, GetProductTest)
{
	EXPECT_EQ(compid_database::get_product({}),
		(compid_database::product_type_info{
			compid_database::product_type::unmarked_object, true }));
	EXPECT_EQ(compid_database::get_product({ 31302, 0x105 }),
		(compid_database::product_type_info{
			compid_database::product_type::visual_studio_2022_17_2_0_preview2_1, true }));
	EXPECT_EQ(compid_database::get_product({ 31300, 0x105 }),
		(compid_database::product_type_info{
			compid_database::product_type::visual_studio_2022_17_2_0_preview1_0, false }));
	EXPECT_EQ(compid_database::get_product({ 0, 0x04 }),
		(compid_database::product_type_info{
			compid_database::product_type::visual_studio_6, false }));
	EXPECT_EQ(compid_database::get_product({
		(std::numeric_limits<std::uint16_t>::max)(), 0x105 }),
		(compid_database::product_type_info{
			compid_database::product_type::unknown, true }));
	EXPECT_EQ(compid_database::get_product({ 0x784b, 0x104 }),
		(compid_database::product_type_info{
			compid_database::product_type::visual_studio_2022_17_0_21, false }));
}

namespace
{
constexpr std::string_view unknown("Unknown");
} //namespace

TEST(CompidDatabaseTests, ProductTypeToNameTest2)
{
	for (int i = 0; i != static_cast<int>(
		compid_database::product_type::unknown); ++i)
	{
		EXPECT_NE(compid_database::product_type_to_string(
			static_cast<compid_database::product_type>(i)), unknown);
	}
}

TEST(CompidDatabaseTests, ToolTypeToNameTest2)
{
	for (int i = 0; i != static_cast<int>(
		compid_database::tool_type::unknown); ++i)
	{
		EXPECT_NE(compid_database::tool_type_to_string(
			static_cast<compid_database::tool_type>(i)), unknown);
	}
}
