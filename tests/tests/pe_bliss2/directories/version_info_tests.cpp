#include <array>
#include <memory>
#include <cstddef>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "buffers/input_virtual_buffer.h"
#include "buffers/input_container_buffer.h"
#include "buffers/ref_buffer.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/detail/resources/version_info.h"
#include "pe_bliss2/packed_c_string.h"
#include "pe_bliss2/resources/version_info.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::resources;

TEST(VersionInfoTests, TranslationHash)
{
	translation t1{ 1, 2 };
	translation t2 = t1;
	translation t3{ 1, 3 };
	translation t4{ 2, 2 };
	const auto hasher = std::hash<translation>{};
	EXPECT_EQ(hasher(t1), hasher(t2));
	EXPECT_NE(hasher(t1), hasher(t3));
	EXPECT_NE(hasher(t1), hasher(t4));
}

TEST(VersionInfoTests, FileVersionInfoSubtype)
{
	file_version_info info;
	EXPECT_TRUE(std::holds_alternative<std::monostate>(info.get_file_subtype()));

	info.get_descriptor()->file_type = pe_bliss::detail::resources::vft_font;
	info.get_descriptor()->file_subtype = pe_bliss::detail::resources::vft2_font_truetype;
	auto subtype = info.get_file_subtype();
	const auto* font_subtype = std::get_if<font_file_subtype>(&subtype);
	ASSERT_NE(font_subtype, nullptr);
	EXPECT_EQ(*font_subtype, font_file_subtype::font_truetype);

	info.get_descriptor()->file_type = pe_bliss::detail::resources::vft_drv;
	info.get_descriptor()->file_subtype = pe_bliss::detail::resources::vft2_drv_mouse;
	subtype = info.get_file_subtype();
	const auto* driver_subtype = std::get_if<driver_file_subtype>(&subtype);
	ASSERT_NE(driver_subtype, nullptr);
	EXPECT_EQ(*driver_subtype, driver_file_subtype::mouse);
}

TEST(VersionInfoTests, FileVersionStrings)
{
	file_version_info info;
	EXPECT_EQ(info.get_file_version_string(), "0.0.0.0");
	EXPECT_EQ(info.get_product_version_string(), "0.0.0.0");
	EXPECT_EQ(info.get_file_version_string<wchar_t>(), L"0.0.0.0");
	EXPECT_EQ(info.get_product_version_string<wchar_t>(), L"0.0.0.0");

	info.get_descriptor()->file_version_ls = 0x0005'001bu;
	info.get_descriptor()->file_version_ms = 0x1000'0007u;
	EXPECT_EQ(info.get_file_version_string(), "4096.7.5.27");
	EXPECT_EQ(info.get_product_version_string(), "0.0.0.0");

	info.get_descriptor()->product_version_ls = 0x0005'001bu;
	info.get_descriptor()->product_version_ms = 0x1000'0007u;
	EXPECT_EQ(info.get_product_version_string(), "4096.7.5.27");
}

TEST(VersionInfoTests, VersionStrings)
{
	version_info info;
	expect_throw_pe_error([&] {
		[[maybe_unused]] auto company = info.get_company_name();
	}, version_info_errc::language_does_not_exist);

	static constexpr std::u16string_view company_name(u"company");
	info.get_strings()[{ 1, 2 }][u"CompanyName"] = company_name;
	expect_throw_pe_error([&] {
		[[maybe_unused]] auto company = info.get_company_name();
	}, version_info_errc::language_does_not_exist);
	EXPECT_EQ(info.get_company_name({ { 1, 2 } }), company_name);

	//Translation: neutral, process default
	static constexpr std::u16string_view company_name2(u"company2");
	info.get_strings()[{0x0400u, 0x04b0u}][u"CompanyName"] = company_name2;
	EXPECT_EQ(info.get_company_name({ { 1, 2 } }), company_name);
	EXPECT_EQ(info.get_company_name(), company_name2);

	expect_throw_pe_error([&] {
		[[maybe_unused]] auto description = info.get_file_description();
	}, version_info_errc::key_does_not_exist);
	expect_throw_pe_error([&] {
		[[maybe_unused]] auto description = info.get_file_description({ { 1, 2 } });
	}, version_info_errc::key_does_not_exist);

	EXPECT_EQ(info.get_value_by_key(u"CompanyName"), company_name2);
	EXPECT_EQ(info.get_value_by_key(u"CompanyName", { { 1, 2 } }), company_name);
}

namespace
{
constexpr std::uint8_t file_version_ms = 1u;
constexpr std::uint8_t file_version_ls = 2u;
constexpr std::array file_version_info_data{
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //signature
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //struc_version
	std::byte{file_version_ms}, std::byte{}, std::byte{}, std::byte{}, //file_version_ms
	std::byte{file_version_ls}, std::byte{}, std::byte{}, std::byte{}, //file_version_ls
	std::byte{3}, std::byte{}, std::byte{}, std::byte{}, //product_version_ms
	std::byte{4}, std::byte{}, std::byte{}, std::byte{}, //product_version_ls
};

constexpr std::uint8_t lcid1 = 5u;
constexpr std::uint8_t cpid1 = 6u;
constexpr std::uint8_t lcid2 = 7u;
constexpr std::uint8_t cpid2 = 8u;
constexpr std::array translation_data{
	std::byte{lcid1}, std::byte{}, //lcid 1
	std::byte{cpid1}, std::byte{}, //cpid 1
	std::byte{lcid2}, std::byte{}, //lcid 2
	std::byte{cpid2}, std::byte{}, //cpid 2
};

void validate_strings_and_translations(const version_info_details& info)
{
	EXPECT_THAT(info.get_translations(), (testing::UnorderedElementsAreArray({
		translation{ lcid1, cpid1 }, translation{ lcid2, cpid2 }
	})));

	EXPECT_THAT(info.get_strings(), (testing::UnorderedElementsAreArray({
		std::pair{ translation{ 0xabcdu, 0x5678u },
			string_value_map_type{ { u"String1", u"Value1" }, { u"String2", u"Value2" } } }
	})));
}
} //namespace

TEST(VersionInfoTests, VersionInfoFromBlock)
{
	version_info_block root;
	root.get_key().emplace().value() = u"VS_VERSION_INFO";
	{
		//Create virtual file version info
		auto file_info_buf = std::make_shared<buffers::input_container_buffer>();
		file_info_buf->get_container()
			= std::vector(file_version_info_data.cbegin(), file_version_info_data.cend());
		auto file_info_buf_virtual = std::make_shared<buffers::input_virtual_buffer>(
			std::move(file_info_buf),
			pe_bliss::detail::packed_reflection::get_type_size<
				pe_bliss::detail::resources::vs_fixedfileinfo>()
			- file_info_buf->get_container().size());
		root.get_value().emplace<buffers::ref_buffer>()
			.deserialize(file_info_buf_virtual, false);
	}

	{
		//Create translations
		auto& var_file_info = root.get_children().emplace_back();
		var_file_info.get_key().emplace().value() = u"VarFileInfo";

		auto& translations = var_file_info.get_children().emplace_back();
		translations.get_key().emplace().value() = u"Translation";

		translations.get_value().emplace<buffers::ref_buffer>().copied_data()
			= std::vector(translation_data.begin(), translation_data.end());
	}

	{
		//Create strings
		auto& string_file_info = root.get_children().emplace_back();
		string_file_info.get_key().emplace().value() = u"StringFileInfo";

		auto& language1 = string_file_info.get_children().emplace_back();
		language1.get_key().emplace().value() = u"abcd5678";

		auto& string1 = language1.get_children().emplace_back();
		string1.get_key().emplace().value() = u"String1";
		string1.get_value().emplace<pe_bliss::packed_utf16_c_string>().value() = u"Value1";

		auto& string2 = language1.get_children().emplace_back();
		string2.get_key().emplace().value() = u"String2";
		string2.get_value().emplace<pe_bliss::packed_utf16_c_string>().value() = u"Value2";

		auto& string3 = language1.get_children().emplace_back();
		string3.get_value().emplace<pe_bliss::packed_utf16_c_string>().value() = u"Value3";

		auto& string4 = language1.get_children().emplace_back();
		string4.get_key().emplace().value() = u"String4";

		auto& language2 = string_file_info.get_children().emplace_back();
		language2.get_key().emplace().value() = u"abcx5678";
	}

	{
		auto info = get_version_info(root, { .allow_virtual_data = true });
		expect_contains_errors(info,
			version_info_errc::invalid_strings,
			version_info_errc::invalid_string_translations);
		EXPECT_EQ(info.get_file_version_info().get_descriptor()->file_version_ls, file_version_ls);
		EXPECT_EQ(info.get_file_version_info().get_descriptor()->file_version_ms, file_version_ms);
		validate_strings_and_translations(info);
	}

	{
		auto info = get_version_info(root);
		expect_contains_errors(info,
			version_info_errc::invalid_strings,
			version_info_errc::invalid_string_translations,
			version_info_errc::file_version_info_read_error);
		EXPECT_EQ(info.get_file_version_info().get_descriptor()->file_version_ls, 0u);
		EXPECT_EQ(info.get_file_version_info().get_descriptor()->file_version_ms, 0u);
		validate_strings_and_translations(info);
	}
}
