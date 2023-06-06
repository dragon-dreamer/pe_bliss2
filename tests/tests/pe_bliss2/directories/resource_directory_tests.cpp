#include "gtest/gtest.h"

#include <utility>

#include "pe_bliss2/packed_utf16_string.h"
#include "pe_bliss2/pe_types.h"
#include "pe_bliss2/resources/resource_directory.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::resources;

TEST(ResourceDirectoryTests, IsNamed)
{
	resource_directory_entry entry;
	EXPECT_FALSE(entry.is_named());

	EXPECT_NO_THROW(entry.get_name_or_id().emplace<
		pe_bliss::packed_utf16_string>(u"abc"));
	EXPECT_TRUE(entry.is_named());
	EXPECT_EQ(entry.get_name().value(), u"abc");
	EXPECT_EQ(std::as_const(entry).get_name().value(), u"abc");
	expect_throw_pe_error([&entry] {
		(void)entry.get_id();
	}, resource_directory_errc::entry_does_not_have_id);
	expect_throw_pe_error([&entry] {
		(void)std::as_const(entry).get_id();
	}, resource_directory_errc::entry_does_not_have_id);

	EXPECT_NO_THROW(entry.get_name_or_id().emplace<resource_id_type>(1u));
	EXPECT_FALSE(entry.is_named());
	EXPECT_EQ(entry.get_id(), 1u);
	EXPECT_EQ(std::as_const(entry).get_id(), 1u);
	expect_throw_pe_error([&entry] {
		(void)entry.get_name();
	}, resource_directory_errc::entry_does_not_have_name);
	expect_throw_pe_error([&entry] {
		(void)std::as_const(entry).get_name();
	}, resource_directory_errc::entry_does_not_have_name);
}

TEST(ResourceDirectoryTests, HasData)
{
	resource_directory_entry entry;
	EXPECT_FALSE(entry.has_data());

	EXPECT_NO_THROW(entry.get_data_or_directory().emplace<
		resource_data_entry>());
	EXPECT_TRUE(entry.has_data());
	EXPECT_FALSE(entry.has_directory());
	EXPECT_NO_THROW((void)entry.get_data());
	EXPECT_NO_THROW((void)std::as_const(entry).get_data());
	expect_throw_pe_error([&entry] {
		(void)entry.get_directory();
	}, resource_directory_errc::entry_does_not_contain_directory);
	expect_throw_pe_error([&entry] {
		(void)std::as_const(entry).get_directory();
	}, resource_directory_errc::entry_does_not_contain_directory);

	EXPECT_NO_THROW(entry.get_data_or_directory().emplace<
		pe_bliss::rva_type>());
	EXPECT_FALSE(entry.has_data());
	EXPECT_FALSE(entry.has_directory());
	expect_throw_pe_error([&entry] {
		(void)entry.get_data();
	}, resource_directory_errc::entry_does_not_contain_data);
	expect_throw_pe_error([&entry] {
		(void)std::as_const(entry).get_data();
	}, resource_directory_errc::entry_does_not_contain_data);

	EXPECT_NO_THROW(entry.get_data_or_directory().emplace<
		resource_directory>());
	EXPECT_FALSE(entry.has_data());
	EXPECT_TRUE(entry.has_directory());
	EXPECT_NO_THROW((void)entry.get_directory());
	EXPECT_NO_THROW((void)std::as_const(entry).get_directory());
}

TEST(ResourceDirectoryTests, EntryById)
{
	resource_directory dir;
	dir.get_entries().emplace_back().get_name_or_id() = resource_name_type(u"abc");
	dir.get_entries().emplace_back().get_name_or_id() = 1u;
	dir.get_entries().emplace_back().get_name_or_id() = 3u;
	EXPECT_EQ(dir.entry_iterator_by_id(2u), dir.get_entries().end());
	EXPECT_EQ(std::as_const(dir).entry_iterator_by_id(2u), dir.get_entries().cend());
	EXPECT_EQ(dir.entry_iterator_by_id(1u), dir.get_entries().begin() + 1u);
	EXPECT_EQ(std::as_const(dir).entry_iterator_by_id(1u), dir.get_entries().cbegin() + 1u);

	EXPECT_EQ(dir.try_entry_by_id(2u), nullptr);
	ASSERT_NE(dir.try_entry_by_id(1u), nullptr);
	EXPECT_EQ(dir.try_entry_by_id(1u)->get_id(), 1u);
	EXPECT_EQ(std::as_const(dir).try_entry_by_id(2u), nullptr);
	ASSERT_NE(std::as_const(dir).try_entry_by_id(1u), nullptr);
	EXPECT_EQ(std::as_const(dir).try_entry_by_id(1u)->get_id(), 1u);

	expect_throw_pe_error([&dir] {
		(void)dir.entry_by_id(2u);
	}, resource_directory_errc::entry_does_not_exist);
	expect_throw_pe_error([&dir] {
		(void)std::as_const(dir).entry_by_id(2u);
	}, resource_directory_errc::entry_does_not_exist);

	EXPECT_EQ(dir.entry_by_id(1u).get_id(), 1u);
	EXPECT_EQ(std::as_const(dir).entry_by_id(1u).get_id(), 1u);
}

TEST(ResourceDirectoryTests, EntryByName)
{
	resource_directory dir;
	dir.get_entries().emplace_back().get_name_or_id() = 1u;
	dir.get_entries().emplace_back().get_name_or_id() = resource_name_type(u"abc");
	dir.get_entries().emplace_back().get_name_or_id() = 3u;
	EXPECT_EQ(dir.entry_iterator_by_name(u"aaa"), dir.get_entries().end());
	EXPECT_EQ(std::as_const(dir).entry_iterator_by_name(u"aaa"), dir.get_entries().cend());
	EXPECT_EQ(dir.entry_iterator_by_name(u"abc"), dir.get_entries().begin() + 1u);
	EXPECT_EQ(std::as_const(dir).entry_iterator_by_name(u"abc"), dir.get_entries().cbegin() + 1u);

	EXPECT_EQ(dir.try_entry_by_name(u"aaa"), nullptr);
	ASSERT_NE(dir.try_entry_by_name(u"abc"), nullptr);
	EXPECT_EQ(dir.try_entry_by_name(u"abc")->get_name().value(), u"abc");
	EXPECT_EQ(std::as_const(dir).try_entry_by_name(u"aaa"), nullptr);
	ASSERT_NE(std::as_const(dir).try_entry_by_name(u"abc"), nullptr);
	EXPECT_EQ(std::as_const(dir).try_entry_by_name(u"abc")->get_name().value(), u"abc");

	expect_throw_pe_error([&dir] {
		(void)dir.entry_by_name(u"aaa");
	}, resource_directory_errc::entry_does_not_exist);
	expect_throw_pe_error([&dir] {
		(void)std::as_const(dir).entry_by_name(u"aaa");
	}, resource_directory_errc::entry_does_not_exist);

	EXPECT_EQ(dir.entry_by_name(u"abc").get_name().value(), u"abc");
	EXPECT_EQ(std::as_const(dir).entry_by_name(u"abc").get_name().value(), u"abc");
}

TEST(ResourceDirectoryTests, EmplaceDirEntryById)
{
	resource_directory dir;
	auto& entry = dir.try_emplace_entry_by_id(123u, directory_entry_contents::directory);
	EXPECT_EQ(entry.get_id(), 123u);
	ASSERT_TRUE(entry.has_directory());
	
	auto& entry2 = dir.try_emplace_entry_by_id(123u, directory_entry_contents::directory);
	EXPECT_EQ(&entry, &entry2);

	expect_throw_pe_error([&dir] {
		(void)dir.try_emplace_entry_by_id(123u, directory_entry_contents::data);
	}, resource_directory_errc::entry_does_not_contain_data);
}

TEST(ResourceDirectoryTests, EmplaceDataEntryById)
{
	resource_directory dir;
	auto& entry = dir.try_emplace_entry_by_id(123u, directory_entry_contents::data);
	EXPECT_EQ(entry.get_id(), 123u);
	ASSERT_TRUE(entry.has_data());

	auto& entry2 = dir.try_emplace_entry_by_id(123u, directory_entry_contents::data);
	EXPECT_EQ(&entry, &entry2);

	expect_throw_pe_error([&dir] {
		(void)dir.try_emplace_entry_by_id(123u, directory_entry_contents::directory);
	}, resource_directory_errc::entry_does_not_contain_directory);
}

TEST(ResourceDirectoryTests, EmplaceDirEntryByName)
{
	resource_directory dir;
	auto& entry = dir.try_emplace_entry_by_name(u"abc", directory_entry_contents::directory);
	EXPECT_EQ(entry.get_name().value(), u"abc");
	ASSERT_TRUE(entry.has_directory());

	auto& entry2 = dir.try_emplace_entry_by_name(u"abc", directory_entry_contents::directory);
	EXPECT_EQ(&entry, &entry2);

	expect_throw_pe_error([&dir] {
		(void)dir.try_emplace_entry_by_name(u"abc", directory_entry_contents::data);
	}, resource_directory_errc::entry_does_not_contain_data);
}

TEST(ResourceDirectoryTests, EmplaceDataEntryByName)
{
	resource_directory dir;
	auto& entry = dir.try_emplace_entry_by_name(directory_entry_contents::data, u"abc");
	EXPECT_EQ(entry.get_name().value(), u"abc");
	ASSERT_TRUE(entry.has_data());

	auto& entry2 = dir.try_emplace_entry_by_name(directory_entry_contents::data, u"abc");
	EXPECT_EQ(&entry, &entry2);

	expect_throw_pe_error([&dir] {
		(void)dir.try_emplace_entry_by_name(directory_entry_contents::directory, u"abc");
	}, resource_directory_errc::entry_does_not_contain_directory);
}
