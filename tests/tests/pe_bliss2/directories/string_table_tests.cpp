#include "gtest/gtest.h"

#include "pe_bliss2/resources/string_table.h"

using namespace pe_bliss::resources;

TEST(StringTableTests, StrignTableIdToStringId)
{
	EXPECT_EQ(string_table::table_to_string_id(5u, 0u), 0x40u);
	EXPECT_EQ(string_table::table_to_string_id(5u, 15u), 0x4fu);
}

TEST(StringTableTests, StringIdToStringTableId)
{
	EXPECT_EQ(string_table::string_to_table_id(0x40u), 5u);
	EXPECT_EQ(string_table::string_to_table_id(0x4fu), 5u);
}
