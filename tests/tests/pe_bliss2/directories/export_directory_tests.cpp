#include "gtest/gtest.h"

#include <string_view>

#include "pe_bliss2/exports/export_directory.h"
#include "pe_bliss2/exports/exported_address.h"
#include "pe_bliss2/pe_types.h"

using namespace pe_bliss;

TEST(ExportDirectoryTests, EmptyExportDirectory)
{
	exports::export_directory dir;
	EXPECT_EQ(dir.get_first_free_ordinal(), 0u);
	EXPECT_EQ(dir.get_last_free_ordinal(), 0u);
	EXPECT_TRUE(dir.get_library_name().value().empty());
	EXPECT_EQ(dir.symbol_by_name("test"), dir.get_export_list().end());
	EXPECT_EQ(dir.symbol_by_ordinal(0u), dir.get_export_list().end());
}

TEST(ExportDirectoryTests, ExportDirectoryModification)
{
	exports::export_directory dir;

	dir.set_library_name("lib");
	EXPECT_EQ(dir.get_library_name().value(), "lib");

	static constexpr exports::ordinal_type ordinal1 = 0u;
	static constexpr rva_type rva1 = 0x123u;
	static constexpr exports::ordinal_type ordinal2 = 3u;
	static constexpr rva_type rva2 = 0x456u;
	static constexpr std::string_view name2 = "name2";
	static constexpr exports::ordinal_type ordinal3 = 2u;
	static constexpr std::string_view name3 = "name3";
	static constexpr std::string_view fwd_name3 = "fwd_name3";

	{
		auto& sym = dir.add(ordinal1, rva1);
		ASSERT_EQ(dir.get_export_list().size(), 1u);
		EXPECT_EQ(&dir.get_export_list()[0], &sym);
		EXPECT_TRUE(sym.get_names().empty());
		EXPECT_FALSE(sym.get_forwarded_name().has_value());
		EXPECT_EQ(sym.get_rva().get(), rva1);
		EXPECT_EQ(sym.get_rva_ordinal(), ordinal1);
	}

	{
		auto& sym = dir.add(ordinal2, name2, rva2);
		ASSERT_EQ(dir.get_export_list().size(), 2u);
		EXPECT_EQ(&dir.get_export_list()[1], &sym);
		ASSERT_EQ(sym.get_names().size(), 1u);
		ASSERT_TRUE(sym.get_names()[0].get_name().has_value());
		EXPECT_EQ(sym.get_names()[0].get_name().value().value(), name2);
		EXPECT_FALSE(sym.get_forwarded_name().has_value());
		EXPECT_EQ(sym.get_rva().get(), rva2);
		EXPECT_EQ(sym.get_rva_ordinal(), ordinal2);
	}

	{
		auto& sym = dir.add(ordinal3, name3, fwd_name3);
		ASSERT_EQ(dir.get_export_list().size(), 3u);
		EXPECT_EQ(&dir.get_export_list()[2], &sym);
		ASSERT_EQ(sym.get_names().size(), 1u);
		ASSERT_TRUE(sym.get_names()[0].get_name().has_value());
		EXPECT_EQ(sym.get_names()[0].get_name().value().value(), name3);
		ASSERT_TRUE(sym.get_forwarded_name().has_value());
		EXPECT_EQ(sym.get_forwarded_name().value().value(), fwd_name3);
		EXPECT_EQ(sym.get_rva().get(), 0u);
		EXPECT_EQ(sym.get_rva_ordinal(), ordinal3);
	}

	EXPECT_EQ(dir.symbol_by_name("test"), dir.get_export_list().end());
	EXPECT_EQ(dir.symbol_by_ordinal(1u), dir.get_export_list().end());

	auto begin = dir.get_export_list().begin();
	EXPECT_EQ(dir.symbol_by_name(name2), begin + 1);
	EXPECT_EQ(dir.symbol_by_ordinal(ordinal2), begin + 1);

	EXPECT_EQ(dir.get_first_free_ordinal(), 1u);
	EXPECT_EQ(dir.get_last_free_ordinal(), 4u);
}
