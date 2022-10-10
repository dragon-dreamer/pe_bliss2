#include <string>

#include "gtest/gtest.h"

#include "utilities/string.h"

using namespace utilities;

TEST(StringTests, CharIequal)
{
	EXPECT_TRUE(char_iequal('1', '1'));
	EXPECT_TRUE(char_iequal('a', 'A'));
	EXPECT_TRUE(char_iequal('A', 'a'));
	EXPECT_TRUE(char_iequal('a', 'a'));
	EXPECT_TRUE(char_iequal('A', 'A'));
	EXPECT_TRUE(char_iequal('z', 'Z'));
	EXPECT_TRUE(char_iequal('\0', '\0'));
	EXPECT_FALSE(char_iequal('0', '1'));
	EXPECT_FALSE(char_iequal('z', 'x'));
	EXPECT_FALSE(char_iequal('z', 'X'));
}

TEST(StringTests, CharToLower)
{
	EXPECT_EQ(to_lower('1'), '1');
	EXPECT_EQ(to_lower('a'), 'a');
	EXPECT_EQ(to_lower('A'), 'a');
	EXPECT_EQ(to_lower('Z'), 'z');
	EXPECT_EQ(to_lower('\0'), '\0');
}

TEST(StringTests, ToLowerInplace)
{
	std::string str("XyZAbC123");
	to_lower_inplace(str);
	EXPECT_EQ(str, "xyzabc123");

	str.clear();
	to_lower_inplace(str);
	EXPECT_EQ(str, "");
}

TEST(StringTests, Iequal)
{
	EXPECT_TRUE(iequal("xyzabc123", "xyzabc123"));
	EXPECT_TRUE(iequal("xYzAbC123", "XyZaBc123"));
	EXPECT_FALSE(iequal("xYzAbF123", "XyZaBc123"));
}

TEST(StringTests, Trim)
{
	std::string_view s("    xyzabc ");
	trim(s);
	EXPECT_EQ(s, "xyzabc");

	std::string_view s2("     ");
	trim(s2);
	EXPECT_EQ(s2, "");
}
