#include "gtest/gtest.h"

#include "pe_bliss2/imports/import_directory.h"
#include "pe_bliss2/detail/imports/image_import_descriptor.h"

using namespace pe_bliss;

TEST(ImportDirectoryTests, ImportedLibraryBound)
{
	imports::imported_library<std::uint32_t,
		detail::imports::image_import_descriptor> lib;
	EXPECT_FALSE(lib.is_bound());

	lib.set_bound();
	EXPECT_TRUE(lib.is_bound());
	EXPECT_EQ(lib.get_descriptor()->time_date_stamp, 0xffffffffu);

	lib.get_descriptor()->time_date_stamp = 123u;
	EXPECT_FALSE(lib.is_bound());
}

TEST(ImportDirectoryTests, LookupTable)
{
	imports::imported_library<std::uint32_t,
		detail::imports::image_import_descriptor> lib;
	EXPECT_FALSE(lib.has_lookup_table());

	lib.get_descriptor()->address_table = 1u;
	EXPECT_FALSE(lib.has_lookup_table());

	lib.get_descriptor()->lookup_table = 1u;
	EXPECT_FALSE(lib.has_lookup_table());

	lib.get_descriptor()->lookup_table = 2u;
	EXPECT_TRUE(lib.has_lookup_table());

	lib.remove_lookup_table();
	EXPECT_FALSE(lib.has_lookup_table());
}
