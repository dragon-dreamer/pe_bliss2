#include "gtest/gtest.h"

#include "pe_bliss2/packed_utf16_string.h"
#include "pe_bliss2/resources/resource_directory.h"

TEST(ResourceDirectoryTests, IsNamed)
{
	pe_bliss::resources::resource_directory_entry entry;
	EXPECT_FALSE(entry.is_named());
	EXPECT_NO_THROW(entry.get_name_or_id().emplace<
		pe_bliss::packed_utf16_string>());
	EXPECT_TRUE(entry.is_named());
	EXPECT_NO_THROW(entry.get_name_or_id().emplace<
		pe_bliss::resources::resource_id_type>());
	EXPECT_FALSE(entry.is_named());
}
