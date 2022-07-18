#include "gtest/gtest.h"

#include "pe_bliss2/exports/exported_address.h"

using namespace pe_bliss;

TEST(ExportedAddressTests, EmptyExportedName)
{
	exports::exported_name name;
	EXPECT_FALSE(name.get_name().has_value());
	EXPECT_EQ(name.get_name_ordinal().get(), 0u);
	EXPECT_EQ(name.get_name_rva().get(), 0u);
}

TEST(ExportedAddressTests, ExportedName)
{
	exports::exported_name name("test", 1u, 2u);
	ASSERT_TRUE(name.get_name().has_value());
	EXPECT_EQ(name.get_name()->value(), "test");
	EXPECT_EQ(name.get_name_rva().get(), 1u);
	EXPECT_EQ(name.get_name_ordinal().get(), 2u);
}

TEST(ExportedAddressTests, EmptyExportedAddress)
{
	exports::exported_address addr;
	EXPECT_EQ(addr.get_rva_ordinal(), 0u);
	EXPECT_EQ(addr.get_rva().get(), 0u);
	EXPECT_TRUE(addr.get_names().empty());
	EXPECT_FALSE(addr.get_forwarded_name().has_value());
}

TEST(ExportedAddressTests, ExportedAddressSetters)
{
	exports::exported_address addr;
	addr.set_rva_ordinal(1u);
	EXPECT_EQ(addr.get_rva_ordinal(), 1u);

	addr.get_rva().get() = 2u;
	EXPECT_EQ(addr.get_rva().get(), 2u);

	addr.get_names().emplace_back().get_name() = "test";
	ASSERT_EQ(addr.get_names().size(), 1u);
	EXPECT_EQ(addr.get_names().back().get_name()->value(), "test");

	addr.get_forwarded_name().emplace("fwd");
	ASSERT_TRUE(addr.get_forwarded_name().has_value());
	EXPECT_EQ(addr.get_forwarded_name()->value(), "fwd");
}

TEST(ExportedAddressTests, GetForwardedNameInfo)
{
	EXPECT_EQ(exports::get_forwarded_name_info(""),
		exports::forwarded_name_info{});
	EXPECT_EQ(exports::get_forwarded_name_info("func"),
		exports::forwarded_name_info{ .function_name = "func" });
	EXPECT_EQ(exports::get_forwarded_name_info("test.func"),
		(exports::forwarded_name_info{ .library_name = "test",
			.function_name = "func"}));
	EXPECT_EQ(exports::get_forwarded_name_info("test.func.x"),
		(exports::forwarded_name_info{ .library_name = "test",
			.function_name = "func.x" }));
}

