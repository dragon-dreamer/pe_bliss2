#include "pe_bliss2/trustlet/trustlet_policy_metadata.h"
#include "pe_bliss2/trustlet/trustlet_policy_metadata_loader.h"

#include <array>
#include <cstring>

#include "gtest/gtest.h"

#include "pe_bliss2/detail/trustlet/image_policy_metadata.h"
#include "pe_bliss2/exports/export_directory.h"
#include "pe_bliss2/image/image.h"

#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::trustlet;

TEST(TrustletPolicyMetadataTests, IsTrustletEmpty)
{
	image::image instance;
	exports::export_directory_details export_dir;
	ASSERT_EQ(is_trustlet(instance, export_dir), trustlet_check_result::absent_metadata);
}

namespace
{
constexpr std::uint32_t trustlet_metadata_offset = 50u;
constexpr std::uint32_t trustlet_raw_section_offset = 0x1000;

void initialize_trustlet_section(image::image& instance,
	std::string_view name = detail::trustlet::image_policy_section_name)
{
	auto& section = instance.get_section_table().get_section_headers().emplace_back();
	section.set_name(name);
	section.set_rva(0x20000u);
	section.set_pointer_to_raw_data(trustlet_raw_section_offset);
	section.set_virtual_size(0x1000);
	section.set_raw_size(0x1000);
	section.set_characteristics(static_cast<section::section_header::characteristics::value>(
		section::section_header::characteristics::mem_read
		| section::section_header::characteristics::cnt_initialized_data));

	auto& section_data = instance.get_section_data_list().emplace_back();
	section_data.copied_data().resize(0x1000);
	section_data.get_buffer().data()->set_absolute_offset(trustlet_raw_section_offset);
}


void initialize_trustlet_export(exports::export_directory_details& export_dir,
	std::string_view export_name = detail::trustlet::image_policy_metadata_name_win10_16215)
{
	auto& metadata_export = export_dir.get_export_list().emplace_back();
	metadata_export.get_rva() = 0x20000u + trustlet_metadata_offset;
	auto& name = metadata_export.get_names().emplace_back();
	name.get_name().emplace().value().assign(export_name);
}

constexpr std::uint8_t application_id = 123;
constexpr std::uint8_t trustlet_c_string_offset = 0u;
constexpr std::uint8_t trustlet_utf16_c_string_offset = 20u;

constexpr auto example_metadata_bytes = std::to_array<std::uint8_t>({
	1, 0, 0, 0, 0, 0, 0, 0, //version
	application_id, 0, 0, 0, 0, 0, 0, 0,
	//entry 1
	static_cast<std::uint8_t>(detail::trustlet::image_policy_entry_type_boolean), 0, 0, 0,
	static_cast<std::uint8_t>(detail::trustlet::image_policy_id_etw), 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0,
	//entry 2
	static_cast<std::uint8_t>(detail::trustlet::image_policy_entry_type_ansi_string), 0, 0, 0,
	static_cast<std::uint8_t>(detail::trustlet::image_policy_id_crash_dump_key_guid), 0, 0, 0,
	trustlet_c_string_offset, 0x00, 0x02, 0x50, 0, 0, 0, 0, //VA
	//entry 3
	static_cast<std::uint8_t>(detail::trustlet::image_policy_entry_type_unicode_string), 0, 0, 0,
	static_cast<std::uint8_t>(detail::trustlet::image_policy_id_device_id), 0, 0, 0,
	trustlet_utf16_c_string_offset, 0x00, 0x02, 0x50, 0, 0, 0, 0, //VA
	//entry 4
	static_cast<std::uint8_t>(detail::trustlet::image_policy_entry_type_ansi_string), 0, 0, 0,
	static_cast<std::uint8_t>(detail::trustlet::image_policy_id_parent_sd), 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 1, //invalid VA
	//entry 5
	100, 0, 0, 0, // invalid type
	static_cast<std::uint8_t>(detail::trustlet::image_policy_id_debug), 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 1, //invalid VA
	//entry 6
	static_cast<std::uint8_t>(detail::trustlet::image_policy_entry_type_uint16), 0, 0, 0,
	static_cast<std::uint8_t>(detail::trustlet::image_policy_id_scenario_id), 0, 0, 0,
	1, 2, 3, 4, 5, 6, 7, 8,
	//entry 7 (final)
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0 //invalid VA
});

template<typename V, typename T>
void expect_variant(const V& variant, const T& value)
{
	const auto* ptr = std::get_if<T>(&variant);
	ASSERT_NE(ptr, nullptr);
	ASSERT_EQ(*ptr, value);
}
} //namespace

TEST(TrustletPolicyMetadataTests, IsTrustletInvalidSection)
{
	image::image instance;
	exports::export_directory_details export_dir;
	initialize_trustlet_export(export_dir);
	ASSERT_EQ(is_trustlet(instance, export_dir), trustlet_check_result::metadata_in_wrong_section);
}

TEST(TrustletPolicyMetadataTests, IsTrustletInvalidSectionName)
{
	image::image instance;
	initialize_trustlet_section(instance, "xyz");

	exports::export_directory_details export_dir;
	initialize_trustlet_export(export_dir);
	ASSERT_EQ(is_trustlet(instance, export_dir), trustlet_check_result::metadata_in_wrong_section);
}

TEST(TrustletPolicyMetadataTests, IsTrustletInvalidSectionAttributes)
{
	image::image instance;
	initialize_trustlet_section(instance);
	instance.get_section_table().get_section_headers().back().set_executable(true);

	exports::export_directory_details export_dir;
	initialize_trustlet_export(export_dir);
	ASSERT_EQ(is_trustlet(instance, export_dir), trustlet_check_result::invalid_metadata_section_attributes);
}

TEST(TrustletPolicyMetadataTests, IsTrustletTrue)
{
	image::image instance;
	initialize_trustlet_section(instance);

	exports::export_directory_details export_dir;
	initialize_trustlet_export(export_dir);
	ASSERT_EQ(is_trustlet(instance, export_dir), trustlet_check_result::valid_trustlet);
}

TEST(TrustletPolicyMetadataTests, IsTrustletTrue2)
{
	image::image instance;
	initialize_trustlet_section(instance);

	exports::export_directory_details export_dir;
	initialize_trustlet_export(export_dir,
		detail::trustlet::image_policy_metadata_name_win10_16193);
	ASSERT_EQ(is_trustlet(instance, export_dir), trustlet_check_result::valid_trustlet);
}

TEST(TrustletPolicyMetadataTests, LoadNone)
{
	image::image instance;
	exports::export_directory_details export_dir;
	ASSERT_FALSE(load_trustlet_policy_metadata(instance, export_dir).has_value());
}

TEST(TrustletPolicyMetadataTests, LoadValidEmpty)
{
	image::image instance;
	initialize_trustlet_section(instance);

	exports::export_directory_details export_dir;
	initialize_trustlet_export(export_dir,
		detail::trustlet::image_policy_metadata_name_win10_16193);

	auto metadata = load_trustlet_policy_metadata(instance, export_dir);
	ASSERT_TRUE(metadata.has_value());
	expect_contains_errors(*metadata, trustlet_policy_errc::unsupported_version);
	ASSERT_TRUE(metadata->get_entries().empty());
}

TEST(TrustletPolicyMetadataTests, LoadValid)
{
	image::image instance;
	instance.get_optional_header().set_raw_image_base(0x50000000u);
	initialize_trustlet_section(instance);

	{
		std::byte* metadata_bytes = instance.get_section_data_list()
			.back().copied_data().data() + trustlet_metadata_offset;
		std::memcpy(metadata_bytes,
			example_metadata_bytes.data(), example_metadata_bytes.size());

		std::byte* c_string_bytes = instance.get_section_data_list()
			.back().copied_data().data() + trustlet_c_string_offset;
		std::memcpy(c_string_bytes, "abcde", 5u);

		std::byte* unicode_string_bytes = instance.get_section_data_list()
			.back().copied_data().data() + trustlet_utf16_c_string_offset;
		std::memcpy(unicode_string_bytes, "x\0y\0z\0a\0b\0c\0", 12u);
	}

	exports::export_directory_details export_dir;
	initialize_trustlet_export(export_dir,
		detail::trustlet::image_policy_metadata_name_win10_16193);

	auto metadata = load_trustlet_policy_metadata(instance, export_dir);
	ASSERT_TRUE(metadata.has_value());
	expect_contains_errors(*metadata);
	ASSERT_EQ(metadata->get_entries().size(), 6u);
	EXPECT_EQ(metadata->get_descriptor()->version,
		detail::trustlet::image_policy_metadata_version);
	EXPECT_EQ(metadata->get_descriptor()->application_id, application_id);

	EXPECT_EQ(metadata->get_descriptor().get_state().relative_offset(),
		trustlet_metadata_offset);
	EXPECT_EQ(metadata->get_descriptor().get_state().absolute_offset(),
		trustlet_raw_section_offset + trustlet_metadata_offset);

	{
		const auto& entry = metadata->get_entries()[0];
		expect_contains_errors(entry);
		EXPECT_EQ(entry.get_policy(), image_policy_id::etw);
		EXPECT_EQ(entry.get_type(), image_policy_entry_type::boolean);
		expect_variant(entry.get_value(), true);
	}
	{
		const auto& entry = metadata->get_entries()[1];
		expect_contains_errors(entry);
		EXPECT_EQ(entry.get_policy(), image_policy_id::crash_dump_key_guid);
		EXPECT_EQ(entry.get_type(), image_policy_entry_type::ansi_string);
		const auto* str = std::get_if<packed_c_string>(&entry.get_value());
		ASSERT_NE(str, nullptr);
		EXPECT_EQ(str->value(), "abcde");
		EXPECT_EQ(str->get_state().absolute_offset(),
			trustlet_raw_section_offset + trustlet_c_string_offset);
		EXPECT_EQ(str->get_state().relative_offset(),
			trustlet_c_string_offset);
	}
	{
		const auto& entry = metadata->get_entries()[2];
		expect_contains_errors(entry);
		EXPECT_EQ(entry.get_policy(), image_policy_id::device_id);
		EXPECT_EQ(entry.get_type(), image_policy_entry_type::unicode_string);
		const auto* str = std::get_if<packed_utf16_c_string>(&entry.get_value());
		ASSERT_NE(str, nullptr);
		EXPECT_EQ(str->value(), u"xyzabc");
		EXPECT_EQ(str->get_state().absolute_offset(),
			trustlet_raw_section_offset + trustlet_utf16_c_string_offset);
		EXPECT_EQ(str->get_state().relative_offset(),
			trustlet_utf16_c_string_offset);
	}
	{
		const auto& entry = metadata->get_entries()[3];
		expect_contains_errors(entry, trustlet_policy_errc::invalid_string_address);
		EXPECT_EQ(entry.get_policy(), image_policy_id::parent_sd);
		EXPECT_EQ(entry.get_type(), image_policy_entry_type::ansi_string);
	}
	{
		const auto& entry = metadata->get_entries()[4];
		expect_contains_errors(entry, trustlet_policy_errc::unsupported_entry_type);
		EXPECT_EQ(entry.get_policy(), image_policy_id::debug);
		EXPECT_EQ(entry.get_type(), static_cast<image_policy_entry_type>(100));
		expect_variant(entry.get_value(), std::monostate{});
	}
	{
		const auto& entry = metadata->get_entries()[5];
		expect_contains_errors(entry);
		EXPECT_EQ(entry.get_policy(), image_policy_id::scenario_id);
		EXPECT_EQ(entry.get_type(), image_policy_entry_type::uint16);
		expect_variant(entry.get_value(), static_cast<std::uint16_t>(0x0201u));
	}
}
