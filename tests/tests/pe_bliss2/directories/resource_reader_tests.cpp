#include <array>
#include <type_traits>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "buffers/ref_buffer.h"
#include "pe_bliss2/packed_utf16_string.h"
#include "pe_bliss2/resources/lcid.h"
#include "pe_bliss2/resources/resource_reader.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::resources;

TEST(ResourceReaderTests, ListResourceTypes)
{
	resource_directory dir;
	EXPECT_TRUE(list_resource_types(dir).empty());

	dir.get_entries().emplace_back().get_name_or_id() = 1u;
	dir.get_entries().emplace_back().get_name_or_id() = packed_utf16_string(u"abc");
	dir.get_entries().emplace_back().get_name_or_id() = 2u;
	dir.get_entries().emplace_back().get_name_or_id() = 1u;
	auto types = list_resource_types(dir);
	ASSERT_EQ(types.size(), 2u);
	ASSERT_EQ(types[0], resource_type{ 1u });
	ASSERT_EQ(types[1], resource_type{ 2u });
}

TEST(ResourceReaderTests, GetDirectoryByType)
{
	resource_directory dir;
	expect_throw_pe_error([&dir] {
		(void)get_directory_by_type(dir, resource_type::anicursor);
	}, resource_directory_errc::entry_does_not_exist);
	expect_throw_pe_error([&dir] {
		(void)get_directory_by_type(std::as_const(dir), resource_type::anicursor);
	}, resource_directory_errc::entry_does_not_exist);

	auto& entry = dir.get_entries().emplace_back();
	entry.get_name_or_id()
		= static_cast<resource_id_type>(resource_type::anicursor);

	entry.get_data_or_directory() = resource_data_entry{};
	expect_throw_pe_error([&dir] {
		(void)get_directory_by_type(dir, resource_type::anicursor);
	}, resource_directory_errc::entry_does_not_contain_directory);
	expect_throw_pe_error([&dir] {
		(void)get_directory_by_type(std::as_const(dir), resource_type::anicursor);
	}, resource_directory_errc::entry_does_not_contain_directory);

	entry.get_data_or_directory() = resource_directory{};
	const auto& nested_dir = entry.get_directory();
	EXPECT_EQ(&get_directory_by_type(dir, resource_type::anicursor), &nested_dir);
	EXPECT_EQ(&get_directory_by_type(std::as_const(dir),
		resource_type::anicursor), &nested_dir);
}

TEST(ResourceReaderTests, GetDirectoryByName)
{
	resource_directory dir;
	expect_throw_pe_error([&dir] {
		(void)get_directory_by_name(dir, u"abc");
	}, resource_directory_errc::entry_does_not_exist);
	expect_throw_pe_error([&dir] {
		(void)get_directory_by_name(std::as_const(dir), u"abc");
	}, resource_directory_errc::entry_does_not_exist);

	auto& entry = dir.get_entries().emplace_back();
	entry.get_name_or_id() = packed_utf16_string(u"abc");

	entry.get_data_or_directory() = resource_data_entry{};
	expect_throw_pe_error([&dir] {
		(void)get_directory_by_name(dir, u"abc");
	}, resource_directory_errc::entry_does_not_contain_directory);
	expect_throw_pe_error([&dir] {
		(void)get_directory_by_name(std::as_const(dir), u"abc");
	}, resource_directory_errc::entry_does_not_contain_directory);

	entry.get_data_or_directory() = resource_directory{};
	const auto& nested_dir = entry.get_directory();
	EXPECT_EQ(&get_directory_by_name(dir, u"abc"), &nested_dir);
	EXPECT_EQ(&get_directory_by_name(std::as_const(dir), u"abc"),
		&nested_dir);
}

namespace
{
const std::vector resource_data{
	std::byte{1}, std::byte{2}, std::byte{3}
};

template<typename NameOrId>
resource_directory create_resources(resource_type Type,
	NameOrId&& name_or_id, lcid_type lang)
{
	resource_directory dir;

	auto& entry = dir.get_entries().emplace_back();
	entry.get_name_or_id() = static_cast<resource_id_type>(Type);
	entry.get_data_or_directory() = resource_directory{};

	auto& name_dir = entry.get_directory();
	auto& name_entry = name_dir.get_entries().emplace_back();
	if constexpr (std::is_same_v<std::remove_cvref_t<
		std::remove_pointer_t<std::decay_t<NameOrId>>>, char16_t>)
	{
		name_entry.get_name_or_id() = packed_utf16_string(name_or_id);
	}
	else
	{
		name_entry.get_name_or_id() = name_or_id;
	}
	name_entry.get_data_or_directory() = resource_directory{};

	auto& lang_dir = name_entry.get_directory();
	auto& lang_entry = lang_dir.get_entries().emplace_back();
	lang_entry.get_name_or_id() = lang;
	lang_entry.get_data_or_directory() = resource_data_entry{};
	lang_entry.get_data().get_raw_data().copied_data() = resource_data;

	auto& second_lang_entry = lang_dir.get_entries().emplace_back();
	second_lang_entry.get_name_or_id() = lang + 1u;

	return dir;
}
} //namespace

TEST(ResourceReaderTests, GetResourceDataByName)
{
	resource_directory dir;

	expect_throw_pe_error([&dir] {
		(void)get_resource_data_by_name(dir,
			resource_type::accelerator, u"abc", 1u);
	}, resource_directory_errc::entry_does_not_exist);
	expect_throw_pe_error([&dir] {
		(void)get_resource_data_by_name(std::as_const(dir),
			resource_type::accelerator, u"abc", 1u);
	}, resource_directory_errc::entry_does_not_exist);

	dir = create_resources(resource_type::accelerator, u"abc", 1u);
	EXPECT_EQ(get_resource_data_by_name(dir, resource_type::accelerator, u"abc", 1u)
		.copied_data(), resource_data);
	EXPECT_EQ(get_resource_data_by_name(std::as_const(dir),
		resource_type::accelerator, u"abc", 1u)
		.copied_data(), resource_data);

	dir = create_resources(resource_type::accelerator, u"abc", 2u);
	expect_throw_pe_error([&dir] {
		(void)get_resource_data_by_name(dir,
			resource_type::accelerator, u"abc", 1u);
	}, resource_directory_errc::entry_does_not_exist);
}

TEST(ResourceReaderTests, GetResourceDataById)
{
	resource_directory dir;

	expect_throw_pe_error([&dir] {
		(void)get_resource_data_by_id(dir,
			resource_type::accelerator, 2u, 1u);
	}, resource_directory_errc::entry_does_not_exist);
	expect_throw_pe_error([&dir] {
		(void)get_resource_data_by_id(std::as_const(dir),
			resource_type::accelerator, 2u, 1u);
	}, resource_directory_errc::entry_does_not_exist);

	dir = create_resources(resource_type::accelerator, 2u, 1u);
	EXPECT_EQ(get_resource_data_by_id(dir, resource_type::accelerator, 2u, 1u)
		.copied_data(), resource_data);
	EXPECT_EQ(get_resource_data_by_id(std::as_const(dir),
		resource_type::accelerator, 2u, 1u)
		.copied_data(), resource_data);

	dir = create_resources(resource_type::accelerator, u"abc", 2u);
	expect_throw_pe_error([&dir] {
		(void)get_resource_data_by_id(dir,
			resource_type::accelerator, 2u, 1u);
	}, resource_directory_errc::entry_does_not_exist);
}

TEST(ResourceReaderTests, GetResourceDataByNameLangIndex)
{
	resource_directory dir;

	expect_throw_pe_error([&dir] {
		(void)get_resource_data_by_name(dir, 0u,
			resource_type::accelerator, u"abc");
	}, resource_directory_errc::entry_does_not_exist);
	expect_throw_pe_error([&dir] {
		(void)get_resource_data_by_name(std::as_const(dir), 0u,
			resource_type::accelerator, u"abc");
	}, resource_directory_errc::entry_does_not_exist);

	dir = create_resources(resource_type::accelerator, u"abc", 1u);
	EXPECT_EQ(get_resource_data_by_name(dir, 0u, resource_type::accelerator, u"abc")
		.copied_data(), resource_data);
	EXPECT_EQ(get_resource_data_by_name(std::as_const(dir), 0u,
		resource_type::accelerator, u"abc")
		.copied_data(), resource_data);

	expect_throw_pe_error([&dir] {
		(void)get_resource_data_by_name(dir, 3u,
			resource_type::accelerator, u"abc");
	}, resource_directory_errc::entry_does_not_exist);
}

TEST(ResourceReaderTests, GetResourceDataByIdLangIndex)
{
	resource_directory dir;

	expect_throw_pe_error([&dir] {
		(void)get_resource_data_by_id(dir, 0u,
			resource_type::accelerator, 2u);
	}, resource_directory_errc::entry_does_not_exist);
	expect_throw_pe_error([&dir] {
		(void)get_resource_data_by_id(std::as_const(dir), 0u,
			resource_type::accelerator, 2u);
	}, resource_directory_errc::entry_does_not_exist);

	dir = create_resources(resource_type::accelerator, 2u, 1u);
	EXPECT_EQ(get_resource_data_by_id(dir, 0u, resource_type::accelerator, 2u)
		.copied_data(), resource_data);
	EXPECT_EQ(get_resource_data_by_id(std::as_const(dir), 0u,
		resource_type::accelerator, 2u)
		.copied_data(), resource_data);

	expect_throw_pe_error([&dir] {
		(void)get_resource_data_by_id(dir, 3u,
			resource_type::accelerator, 2u);
	}, resource_directory_errc::entry_does_not_exist);
}

TEST(ResourceReaderTests, ForEachResource)
{
	resource_directory dir;

	auto& entry1 = dir.get_entries().emplace_back();
	entry1.get_name_or_id() = static_cast<resource_id_type>(resource_type::aniicon);
	entry1.get_data_or_directory() = resource_directory{};

	auto& name_dir1 = entry1.get_directory();
	auto& name_entry1 = name_dir1.get_entries().emplace_back();
	name_entry1.get_name_or_id() = packed_utf16_string(u"abc");
	name_entry1.get_data_or_directory() = resource_directory{};

	auto& lang_dir1 = name_entry1.get_directory();
	auto& lang_entry1 = lang_dir1.get_entries().emplace_back();
	lang_entry1.get_name_or_id() = 123u;
	lang_entry1.get_data_or_directory() = resource_data_entry{};
	lang_entry1.get_data().get_raw_data().copied_data() = resource_data;

	auto& lang_entry2 = lang_dir1.get_entries().emplace_back();
	lang_entry2.get_name_or_id() = packed_utf16_string(u"xxx");
	lang_entry2.get_data_or_directory() = resource_data_entry{};
	lang_entry2.get_data().get_raw_data().copied_data() = resource_data;

	auto& lang_entry3 = lang_dir1.get_entries().emplace_back();
	lang_entry3.get_name_or_id() = 111u;

	auto& name_dir2 = entry1.get_directory();
	auto& name_entry2 = name_dir2.get_entries().emplace_back();
	name_entry2.get_name_or_id() = 456u;
	name_entry2.get_data_or_directory() = resource_directory{};

	const std::vector resource_data2{
		std::byte{3}, std::byte{2}, std::byte{1}
	};
	auto& lang_dir2 = name_entry2.get_directory();
	auto& lang_entry4 = lang_dir2.get_entries().emplace_back();
	lang_entry4.get_name_or_id() = 789u;
	lang_entry4.get_data_or_directory() = resource_data_entry{};
	lang_entry4.get_data().get_raw_data().copied_data() = resource_data2;

	using namespace testing;

	{
		MockFunction<bool(resource_directory_entry::name_or_id_type&,
			lcid_type, buffers::ref_buffer&)> mockCallback;

		InSequence seq;
		EXPECT_CALL(mockCallback, Call(VariantWith<
			resource_name_type>(resource_name_type(u"abc")),
			123u,
			Property(&buffers::ref_buffer::copied_data, resource_data)))
			.WillOnce(Return(false));
		EXPECT_CALL(mockCallback, Call(VariantWith<resource_id_type>(456u),
			789u,
			Property(&buffers::ref_buffer::copied_data, resource_data2)))
			.WillOnce(Return(false));

		EXPECT_FALSE(for_each_resource(dir,
			resource_type::aniicon, mockCallback.AsStdFunction()));
	}

	{
		MockFunction<bool(const resource_directory_entry::name_or_id_type&,
			lcid_type, const buffers::ref_buffer&)> mockCallback;

		InSequence seq;
		EXPECT_CALL(mockCallback, Call(VariantWith<
			resource_name_type>(resource_name_type(u"abc")),
			123u,
			Property(&buffers::ref_buffer::copied_data, resource_data)))
			.WillOnce(Return(true));

		EXPECT_TRUE(for_each_resource(std::as_const(dir),
			resource_type::aniicon, mockCallback.AsStdFunction()));
	}
}
