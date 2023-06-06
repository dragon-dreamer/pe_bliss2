#include <cstddef>
#include <memory>
#include <variant>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "buffers/input_memory_buffer.h"
#include "pe_bliss2/resources/manifest.h"
#include "pe_bliss2/resources/manifest_accessor_interface.h"
#include "pe_bliss2/resources/pugixml_manifest_accessor.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::resources;

TEST(ManifestTests, AssemblyVersion)
{
	EXPECT_TRUE((assembly_version{ 1,2,3,4 }.is_compatible_with({ 1,2,5,6 })));
	EXPECT_FALSE((assembly_version{ 1,2,3,4 }.is_compatible_with({ 1,3,3,4 })));
	EXPECT_FALSE((assembly_version{ 1,2,3,4 }.is_compatible_with({ 2,2,3,4 })));
}

TEST(ManifestTests, AssemblyIdentityGetType)
{
	assembly_identity identity;
	EXPECT_EQ(identity.get_type(), assembly_identity_type::unknown);

	identity.get_type_raw() = "win32";
	EXPECT_EQ(identity.get_type(), assembly_identity_type::win32);
}

namespace
{
void ensure_en_us_lcid(const std::optional<lcid_info>& lang)
{
	ASSERT_TRUE(lang);
	EXPECT_EQ(lang->language, lcid_language::english);
	EXPECT_EQ(lang->location, "United States");
	EXPECT_EQ(lang->lcid, 0x0409u);
}
} //namespace

TEST(ManifestTests, AssemblyIdentityGetLanguage)
{
	assembly_identity identity;
	EXPECT_FALSE(identity.get_language());

	identity.get_language_raw() = "*";
	auto lang = identity.get_language();
	ASSERT_TRUE(lang);
	EXPECT_EQ(lang->language, lcid_language::neutral);
	EXPECT_EQ(lang->lcid, 0u);

	identity.get_language_raw() = "en-us";
	ensure_en_us_lcid(identity.get_language());
	identity.get_language_raw() = "En-uS";
	ensure_en_us_lcid(identity.get_language());
}

TEST(ManifestTests, AssemblyIdentityGetProcessorArchitecture)
{
	assembly_identity identity;
	EXPECT_EQ(identity.get_processor_architecture(),
		assembly_processor_architecture::unspecified);

	identity.get_processor_architecture_raw() = "*";
	EXPECT_EQ(identity.get_processor_architecture(),
		assembly_processor_architecture::any);

	identity.get_processor_architecture_raw() = "amd64";
	EXPECT_EQ(identity.get_processor_architecture(),
		assembly_processor_architecture::amd64);

	identity.get_processor_architecture_raw() = "xxx";
	EXPECT_EQ(identity.get_processor_architecture(),
		assembly_processor_architecture::unknown);
}

TEST(ManifestTests, AssemblyIdentityGetVersion)
{
	assembly_identity identity;
	expect_throw_pe_error([&identity] {
		(void)identity.get_version();
	}, manifest_errc::invalid_version_string);

	identity.get_version_raw() = "1.2.c.d";
	expect_throw_pe_error([&identity] {
		(void)identity.get_version();
	}, manifest_errc::invalid_version_string);

	identity.get_version_raw() = "1.2.3.";
	expect_throw_pe_error([&identity] {
		(void)identity.get_version();
	}, manifest_errc::invalid_version_string);

	identity.get_version_raw() = "1.2.3.4";
	EXPECT_EQ(identity.get_version(), (assembly_version{ 1,2,3,4 }));
}

TEST(ManifestTests, AssemblyIdentityGetPublicKeyToken)
{
	assembly_identity identity;
	EXPECT_FALSE(identity.get_public_key_token());

	identity.get_public_key_token_raw() = "abcdef";
	expect_throw_pe_error([&identity] {
		(void)identity.get_public_key_token();
	}, manifest_errc::invalid_hex_string);

	identity.get_public_key_token_raw() = "abCD123456def012";
	auto token = identity.get_public_key_token();
	ASSERT_TRUE(token);
	EXPECT_EQ(*token, (public_key_token_type{
		std::byte{0xabu}, std::byte{0xcdu}, std::byte{0x12u},
		std::byte{0x34u}, std::byte{0x56u}, std::byte{0xdeu},
		std::byte{0xf0u}, std::byte{0x12u} }));

	identity.get_public_key_token_raw() = "abCD123456deg012";
	expect_throw_pe_error([&identity] {
		(void)identity.get_public_key_token();
	}, manifest_errc::invalid_hex_string);
}

TEST(ManifestTests, AssemblySupportedOsListBaseGetList)
{
	assembly_supported_os_list list;
	EXPECT_TRUE(list.get_list().empty());

	list.get_list_raw().emplace_back("{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}");
	list.get_list_raw().emplace_back("{4A2f28E3-53b9-4441-ba9c-d69d4a4a6e38}");
	EXPECT_THAT(list.get_list(), ::testing::UnorderedElementsAre(
		assembly_supported_os::win10_win11_server2016_server2019_server2022,
		assembly_supported_os::win8_server2012
	));
	
	//add duplicate element
	list.get_list_raw().emplace_back("{4A2f28E3-53b9-4441-ba9c-d69d4a4a6e38}");
	EXPECT_THAT(list.get_list(), ::testing::UnorderedElementsAre(
		assembly_supported_os::win10_win11_server2016_server2019_server2022,
		assembly_supported_os::win8_server2012
	));

	list.get_list_raw().emplace_back("{4A2f28E3-53bx-4441-ba9c-d69d4a4a6e38}");
	EXPECT_THAT(list.get_list(), ::testing::UnorderedElementsAre(
		assembly_supported_os::win10_win11_server2016_server2019_server2022,
		assembly_supported_os::win8_server2012,
		assembly_supported_os::unsupported
	));
}

TEST(ManifestTests, AssemblySupportedOsListBaseGetMaxTestedOsVersion)
{
	assembly_supported_os_list list;
	EXPECT_FALSE(list.get_max_tested_os_version());

	list.get_max_tested_os_version_raw() = "55.66.77.88";
	EXPECT_EQ(list.get_max_tested_os_version(), (full_version{ 55,66,77,88 }));

	list.get_max_tested_os_version_raw() = "55.66.77.88.";
	expect_throw_pe_error([&list] {
		(void)list.get_max_tested_os_version();
	}, manifest_errc::invalid_version_string);
}

TEST(ManifestTests, ComProgidIsValid)
{
	com_progid id;
	EXPECT_FALSE(id.is_valid());

	id.get_component() = "Test";
	id.get_program() = "Program";
	EXPECT_TRUE(id.is_valid());

	id.get_version() = "123";
	EXPECT_TRUE(id.is_valid());

	id.get_component() = "Test$";
	EXPECT_FALSE(id.is_valid());
}

TEST(ManifestTests, ComProgidParse)
{
	expect_throw_pe_error([] {
		(void)com_progid::parse("xxxx");
	}, manifest_errc::invalid_com_progid);

	expect_throw_pe_error([] {
		(void)com_progid::parse("xxxx.xxxx.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
	}, manifest_errc::invalid_com_progid);

	expect_throw_pe_error([] {
		(void)com_progid::parse("a.b.c.d.e");
	}, manifest_errc::invalid_com_progid);

	expect_throw_pe_error([] {
		(void)com_progid::parse("a.b.");
	}, manifest_errc::invalid_com_progid);

	EXPECT_EQ(com_progid::parse("abcd.efg.123"),
		(com_progid{ "abcd", "efg", "123" }));
	EXPECT_EQ(com_progid::parse("abcd.efg"),
		(com_progid{ "abcd", "efg" }));
}

namespace
{
constexpr guid expected_guid{ 0x8e0f7a12u, 0xbfb3u, 0x4fe8u,
		0xb9u, 0xa5u, 0x48u, 0xfdu, 0x50u, 0xa1u, 0x5au, 0x9au };
} //namespace

TEST(ManifestTests, ComClassClsid)
{
	com_class obj;

	expect_throw_pe_error([&obj] {
		(void)obj.get_clsid();
	}, guid_errc::invalid_guid);

	obj.get_clsid_raw() = "{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}";
	EXPECT_EQ(obj.get_clsid(), expected_guid);
}

TEST(ManifestTests, ComClassTlbid)
{
	com_class obj;
	EXPECT_FALSE(obj.get_tlbid());

	obj.get_tlbid_raw() = "{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}";
	EXPECT_EQ(obj.get_tlbid(), expected_guid);
}

TEST(ManifestTests, ComClassProgid)
{
	com_class obj;
	EXPECT_FALSE(obj.get_progid());

	obj.get_progid_raw() = "a.b";
	EXPECT_EQ(obj.get_progid(), (com_progid{ "a", "b" }));
}

namespace
{
template<typename T>
void test_threading_model(T& obj)
{
	EXPECT_EQ(obj.get_threading_model(), com_threading_model::unspecified);

	obj.get_threading_model_raw() = "Apartment";
	EXPECT_EQ(obj.get_threading_model(), com_threading_model::apartment);
	obj.get_threading_model_raw() = "Both";
	EXPECT_EQ(obj.get_threading_model(), com_threading_model::both);
	obj.get_threading_model_raw() = "Free";
	EXPECT_EQ(obj.get_threading_model(), com_threading_model::free);
	obj.get_threading_model_raw() = "Neutral";
	EXPECT_EQ(obj.get_threading_model(), com_threading_model::neutral);
	obj.get_threading_model_raw() = "neutral";
	EXPECT_EQ(obj.get_threading_model(), com_threading_model::neutral);
	obj.get_threading_model_raw() = "abc";
	EXPECT_EQ(obj.get_threading_model(), com_threading_model::unknown);
}
} //namespace

TEST(ManifestTests, ComClassThreadingModel)
{
	com_class obj;
	test_threading_model(obj);
}

namespace
{
template<auto Getter,
	std::optional<std::string>& (com_class::* Setter)()&,
	typename ComClass>
void test_misc_class(ComClass& obj)
{
	EXPECT_EQ((obj.*Getter)(), 0u);
	(obj.*Setter)() = "recomposeonresize,imemode , wantstomenumerge";
	EXPECT_EQ((obj.*Getter)(), ole_misc::recomposeonresize
		| ole_misc::imemode | ole_misc::wantstomenumerge);

	(obj.*Setter)() = "recomposeonresize,imemode , wantstomenumerge, ";
	EXPECT_EQ((obj.*Getter)(), ole_misc::recomposeonresize
		| ole_misc::imemode | ole_misc::wantstomenumerge | ole_misc::unknown);
}
} //namespace

TEST(ManifestTests, ComClassMiscStatus)
{
	com_class obj;
	test_misc_class<&com_class::get_misc_status,
		&com_class::get_misc_status_raw>(obj);
	test_misc_class<&com_class::get_misc_status_content,
		&com_class::get_misc_status_content_raw>(obj);
	test_misc_class<&com_class::get_misc_status_docprint,
		&com_class::get_misc_status_docprint_raw>(obj);
	test_misc_class<&com_class::get_misc_status_icon,
		&com_class::get_misc_status_icon_raw>(obj);
	test_misc_class<&com_class::get_misc_status_thumbnail,
		&com_class::get_misc_status_thumbnail_raw>(obj);
}

TEST(ManifestTests, FullVersion)
{
	expect_throw_pe_error([] {
		(void)parse_full_version("1.2.3.4.");
	}, manifest_errc::invalid_version_string);
	expect_throw_pe_error([] {
		(void)parse_full_version("1.2.xx.4");
	}, manifest_errc::invalid_version_string);
	expect_throw_pe_error([] {
		(void)parse_full_version("1.2.3");
	}, manifest_errc::invalid_version_string);
	expect_throw_pe_error([] {
		(void)parse_full_version("1.2.3.70000");
	}, manifest_errc::invalid_version_string);
	expect_throw_pe_error([] {
		(void)parse_full_version("1.-2.3.4");
	}, manifest_errc::invalid_version_string);

	EXPECT_EQ(parse_full_version("11.33.55.77"),
		(full_version{ 11, 33, 55, 77 }));
}

TEST(ManifestTests, ShortVersion)
{
	expect_throw_pe_error([] {
		(void)parse_short_version("1.2.");
	}, manifest_errc::invalid_version_string);
	expect_throw_pe_error([] {
		(void)parse_short_version("1.xx");
	}, manifest_errc::invalid_version_string);
	expect_throw_pe_error([] {
		(void)parse_short_version("1");
	}, manifest_errc::invalid_version_string);
	expect_throw_pe_error([] {
		(void)parse_short_version("1.70000");
	}, manifest_errc::invalid_version_string);
	expect_throw_pe_error([] {
		(void)parse_short_version("1.-2");
	}, manifest_errc::invalid_version_string);

	EXPECT_EQ(parse_short_version("500.700"),
		(short_version{ 500, 700 }));
}

TEST(ManifestTests, ComTypelibTlbid)
{
	com_typelib obj;
	expect_throw_pe_error([&obj] {
		(void)obj.get_tlbid();
	}, guid_errc::invalid_guid);

	obj.get_tlbid_raw() = "{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}";
	EXPECT_EQ(obj.get_tlbid(), expected_guid);
}

TEST(ManifestTests, ComTypelibShortVersion)
{
	com_typelib obj;
	expect_throw_pe_error([&obj] {
		(void)obj.get_version();
	}, manifest_errc::invalid_version_string);

	obj.get_version_raw() = "11.234";
	EXPECT_EQ(obj.get_version(), (short_version{ 11, 234 }));
}

TEST(ManifestTests, ComTypelibResourceId)
{
	com_typelib obj;
	EXPECT_FALSE(obj.get_resource_id());

	obj.get_resource_id_raw() = "23bce";
	expect_throw_pe_error([&obj] {
		(void)obj.get_resource_id();
	}, manifest_errc::invalid_lcid_string);

	obj.get_resource_id_raw() = "23b";
	EXPECT_EQ(obj.get_resource_id(), 0x23bu);
}

TEST(ManifestTests, ComTypelibFlags)
{
	com_typelib obj;
	EXPECT_FALSE(obj.get_flags());

	obj.get_flags_raw() = "hasdiskimage, control";
	EXPECT_EQ(obj.get_flags(),
		com_typelib_flags::has_disk_image | com_typelib_flags::control);

	obj.get_flags_raw() = "hasdiskimage, xxx";
	EXPECT_EQ(obj.get_flags(),
		com_typelib_flags::has_disk_image | com_typelib_flags::unknown);

	obj.get_flags_raw() = "";
	EXPECT_EQ(obj.get_flags(), 0u);
}

TEST(ManifestTests, ComInterfaceExternalProxyStubIid)
{
	com_interface_external_proxy_stub obj;
	expect_throw_pe_error([&obj] {
		(void)obj.get_iid();
	}, guid_errc::invalid_guid);

	obj.get_iid_raw() = "{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}";
	EXPECT_EQ(obj.get_iid(), expected_guid);
}

TEST(ManifestTests, ComInterfaceExternalProxyStubBaseInterface)
{
	com_interface_external_proxy_stub obj;
	EXPECT_FALSE(obj.get_base_interface());

	obj.get_base_interface_raw() = "{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}";
	EXPECT_EQ(obj.get_base_interface(), expected_guid);
}

TEST(ManifestTests, ComInterfaceExternalProxyStubTlbid)
{
	com_interface_external_proxy_stub obj;
	EXPECT_FALSE(obj.get_tlbid());

	obj.get_tlbid_raw() = "{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}";
	EXPECT_EQ(obj.get_tlbid(), expected_guid);
}

TEST(ManifestTests, ComInterfaceExternalProxyStubProxyStubClsid32)
{
	com_interface_external_proxy_stub obj;
	EXPECT_FALSE(obj.get_proxy_stub_clsid32());

	obj.get_proxy_stub_clsid32_raw() = "{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}";
	EXPECT_EQ(obj.get_proxy_stub_clsid32(), expected_guid);
}

TEST(ManifestTests, ComInterfaceExternalProxyNumMethods)
{
	com_interface_external_proxy_stub obj;
	EXPECT_FALSE(obj.get_num_methods());

	obj.get_num_methods_raw() = "12345678912345";
	EXPECT_EQ(obj.get_num_methods(), 12345678912345ull);

	obj.get_num_methods_raw() = "-12345678912345";
	expect_throw_pe_error([&obj] {
		(void)obj.get_num_methods();
	}, manifest_errc::invalid_num_methods_string);
}

TEST(ManifestTests, ComInterfaceProxyStubThreadingModel)
{
	com_interface_proxy_stub obj;
	test_threading_model(obj);
}

TEST(ManifestTests, WindowClassIsVersioned)
{
	window_class obj;
	EXPECT_TRUE(obj.is_versioned());

	obj.is_versioned_raw() = "yes";
	EXPECT_TRUE(obj.is_versioned());

	obj.is_versioned_raw() = "no";
	EXPECT_FALSE(obj.is_versioned());

	obj.is_versioned_raw() = "xxx";
	expect_throw_pe_error([&obj] {
		(void)obj.is_versioned();
	}, manifest_errc::invalid_bool_string);
}

TEST(ManifestTests, AssemblyFileHashAlgorithm)
{
	assembly_file file;
	EXPECT_EQ(file.get_hash_algorithm(), assembly_file_hash_algorithm::unspecified);

	file.get_hash_algorithm_raw() = "ShA1";
	EXPECT_EQ(file.get_hash_algorithm(), assembly_file_hash_algorithm::sha1);

	file.get_hash_algorithm_raw() = "sHa256";
	EXPECT_EQ(file.get_hash_algorithm(), assembly_file_hash_algorithm::sha256);

	file.get_hash_algorithm_raw() = "xxx";
	EXPECT_EQ(file.get_hash_algorithm(), assembly_file_hash_algorithm::unknown);
}

TEST(ManifestTests, AssemblyFileHash)
{
	assembly_file file;
	EXPECT_FALSE(file.get_hash());

	file.get_hash_raw() = "aab";
	expect_throw_pe_error([&file] {
		(void)file.get_hash();
	}, manifest_errc::invalid_hex_string);

	file.get_hash_raw() = "aabbcczz";
	expect_throw_pe_error([&file] {
		(void)file.get_hash();
	}, manifest_errc::invalid_hex_string);

	file.get_hash_raw() = "aabbccdd01";
	EXPECT_EQ(file.get_hash(), (std::vector{
		std::byte{0xaau}, std::byte{0xbbu},
		std::byte{0xccu}, std::byte{0xddu}, std::byte{0x1u} }));
}

TEST(ManifestTests, AssemblyFileSize)
{
	assembly_file file;
	EXPECT_FALSE(file.get_size());

	file.get_size_raw() = "12345";
	EXPECT_EQ(file.get_size(), 12345u);

	file.get_size_raw() = "-12345";
	expect_throw_pe_error([&file] {
		(void)file.get_size();
	}, manifest_errc::invalid_file_size_string);
}

TEST(ManifestTests, ActiveCodePage)
{
	active_code_page cp;
	EXPECT_TRUE(std::holds_alternative<std::monostate>(cp.get_code_page()));

	cp.get_name() = "UtF-8";
	EXPECT_TRUE(std::holds_alternative<utf8_code_page_tag>(cp.get_code_page()));

	cp.get_name() = "LEGAcy";
	EXPECT_TRUE(std::holds_alternative<legacy_code_page_tag>(cp.get_code_page()));

	cp.get_name() = "en-Gb";
	auto page_variant = cp.get_code_page();
	const auto* page = std::get_if<lcid_info>(&page_variant);
	ASSERT_NE(page, nullptr);
	EXPECT_EQ(page->lcid, 0x809u);
}

TEST(ManifestTests, DpiAwarenessAware)
{
	dpi_awareness dpi;
	EXPECT_EQ(dpi.get_dpi_aware_value(), dpi_aware_value::absent);

	dpi.get_dpi_aware_raw() = "True";
	EXPECT_EQ(dpi.get_dpi_aware_value(), dpi_aware_value::dpi_aware);

	dpi.get_dpi_aware_raw() = "True/Pm";
	EXPECT_EQ(dpi.get_dpi_aware_value(), dpi_aware_value::dpi_aware_true_per_monitor);

	dpi.get_dpi_aware_raw() = "per Monitor";
	EXPECT_EQ(dpi.get_dpi_aware_value(), dpi_aware_value::dpi_aware_per_monitor);

	dpi.get_dpi_aware_raw() = "false";
	EXPECT_EQ(dpi.get_dpi_aware_value(), dpi_aware_value::dpi_unaware);

	dpi.get_dpi_aware_raw() = "xxx";
	EXPECT_EQ(dpi.get_dpi_aware_value(), dpi_aware_value::absent);
}

TEST(ManifestTests, DpiAwareness)
{
	dpi_awareness dpi;
	EXPECT_EQ(dpi.get_dpi_awareness_value(), dpi_awareness_value::absent);

	dpi.get_dpi_awareness_raw() = "SysTem";
	EXPECT_EQ(dpi.get_dpi_awareness_value(), dpi_awareness_value::dpi_aware_system);

	dpi.get_dpi_awareness_raw() = "zzz, aaa, permonitor, permonitorv2";
	EXPECT_EQ(dpi.get_dpi_awareness_value(), dpi_awareness_value::dpi_aware_per_monitor);

	dpi.get_dpi_awareness_raw() = "permonitorV2";
	EXPECT_EQ(dpi.get_dpi_awareness_value(), dpi_awareness_value::dpi_aware_per_monitor_v2);

	dpi.get_dpi_awareness_raw() = "unaware";
	EXPECT_EQ(dpi.get_dpi_awareness_value(), dpi_awareness_value::dpi_unaware);

	dpi.get_dpi_awareness_raw() = "aaa, bbb, ccc,,,";
	EXPECT_EQ(dpi.get_dpi_awareness_value(), dpi_awareness_value::unrecognized);
}

TEST(ManifestTests, HeapType)
{
	heap_type type;
	EXPECT_EQ(type.get_heap_type(), heap_type_value::unknown);

	type.get_heap_type_raw() = "SEGMENTHeap";
	EXPECT_EQ(type.get_heap_type(), heap_type_value::segment_heap);
}

TEST(ManifestTests, RequestedPrivilegesLevel)
{
	requested_privileges priv;
	EXPECT_EQ(priv.get_level(), requested_execution_level::unknown);

	priv.get_level_raw() = "requireAdministrator";
	EXPECT_EQ(priv.get_level(), requested_execution_level::require_administrator);

	priv.get_level_raw() = "highestAvailable";
	EXPECT_EQ(priv.get_level(), requested_execution_level::highest_available);

	priv.get_level_raw() = "asInvoker";
	EXPECT_EQ(priv.get_level(), requested_execution_level::as_invoker);
}

TEST(ManifestTests, RequestedPrivilegesUiAccess)
{
	requested_privileges priv;
	EXPECT_FALSE(priv.get_ui_access());

	priv.get_ui_access_raw() = "true";
	EXPECT_EQ(priv.get_ui_access(), true);

	priv.get_ui_access_raw() = "false";
	EXPECT_EQ(priv.get_ui_access(), false);
}

namespace
{
constexpr std::string_view empty_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" />)"
);

constexpr std::string_view assembly_identity_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<!-- Some comment -->)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(</assembly>)"
);

constexpr std::string_view assembly_identity_wrong_index_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<someElem />)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(</assembly>)"
);

constexpr std::string_view empty_assembly_identity_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<assemblyIdentity />)"
	R"(</assembly>)"
);

constexpr std::string_view windows_settings_manifest_header(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(<asmv3:application xmlns:asmv3="urn:schemas-microsoft-com:asm.v3">")"
	R"(<asmv3:windowsSettings xmlns:ws05="http://schemas.microsoft.com/SMI/2005/WindowsSettings")"
	R"( xmlns:ws11="http://schemas.microsoft.com/SMI/2011/WindowsSettings")"
	R"( xmlns:ws13="http://schemas.microsoft.com/SMI/2013/WindowsSettings")"
	R"( xmlns:ws16="http://schemas.microsoft.com/SMI/2016/WindowsSettings")"
	R"( xmlns:ws17="http://schemas.microsoft.com/SMI/2017/WindowsSettings")"
	R"( xmlns:ws19="http://schemas.microsoft.com/SMI/2019/WindowsSettings")"
	R"( xmlns:ws20="http://schemas.microsoft.com/SMI/2020/WindowsSettings">)"
);

constexpr std::string_view windows_settings_manifest_footer(
	R"(</asmv3:windowsSettings>)"
	R"(</asmv3:application>)"
	R"(</assembly>)"
);

constexpr std::string_view multiple_windows_settings_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(<asmv3:application xmlns:asmv3="urn:schemas-microsoft-com:asm.v3">")"
	R"(<asmv3:windowsSettings />")
	R"(<asmv3:windowsSettings />")
	R"(</asmv3:application>)"
	R"(</assembly>)"
);

constexpr std::string_view description_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(<description>Description line 1</description>)"
	R"(<description>Description line 2</description>)"
	R"(</assembly>)"
);

constexpr std::string_view msix_identity_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(<msix xmlns="urn:schemas-microsoft-com:msix.v1"
          publisher="Publisher"
          packageName="Package"
          applicationId="Application"
        />)"
	R"(</assembly>)"
);

constexpr std::string_view empty_msix_identity_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(<msix xmlns="urn:schemas-microsoft-com:msix.v1" />)"
	R"(</assembly>)"
);

constexpr std::string_view multiple_msix_identity_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(<msix xmlns="urn:schemas-microsoft-com:msix.v1" />)"
	R"(<msix xmlns="urn:schemas-microsoft-com:msix.v1" />)"
	R"(</assembly>)"
);

constexpr std::string_view no_inherit_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<noInherit />)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(</assembly>)"
);

constexpr std::string_view no_inheritable_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<noInheritable />)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(</assembly>)"
);

constexpr std::string_view multiple_no_inherit_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<noInherit />)"
	R"(<noInheritable />)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(</assembly>)"
);

constexpr std::string_view wrong_no_inherit_pos_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(<noInherit />)"
	R"(</assembly>)"
);

constexpr std::string_view no_inherit_wrong_assembly_identity_pos_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<noInherit />)"
	R"(<something />)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(</assembly>)"
);

constexpr std::string_view supported_os_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(<compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
		<application>
			<!-- Windows 10 and Windows 11 -->
			<supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}"/>
			<!-- Windows 8.1 -->
			<supportedOS Id="{1f676c76-80e1-4239-95bb-83d0f6d0da78}"/>
			<!-- Windows 8 -->
			<supportedOS Id="{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}"/>
			<!-- Windows 7 -->
			<supportedOS Id="{35138b9a-5d96-4fbd-8e2d-a2440225f93a}"/>
			<!-- Windows Vista -->
			<supportedOS Id="{e2011457-1546-43c5-a5fe-008deee3d3f0}"/>
			<maxversiontested Id="10.0.19041.0"/>
		</application>
	</compatibility>)"
	R"(</assembly>)"
);

constexpr std::string_view supported_os_manifest_multiple_compatibility(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(<compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1" />")
	R"(<compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
		<application>
			<!-- Windows 10 and Windows 11 -->
			<supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}"/>
			<!-- Windows 8.1 -->
			<supportedOS Id="{1f676c76-80e1-4239-95bb-83d0f6d0da78}"/>
			<!-- Windows 8 -->
			<supportedOS Id="{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}"/>
			<!-- Windows 7 -->
			<supportedOS Id="{35138b9a-5d96-4fbd-8e2d-a2440225f93a}"/>
			<!-- Windows Vista -->
			<supportedOS Id="{e2011457-1546-43c5-a5fe-008deee3d3f0}"/>
			<maxversiontested Id="10.0.19041.0"/>
		</application>
	</compatibility>)"
	R"(</assembly>)"
);

constexpr std::string_view trust_info_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(<trustInfo xmlns="urn:schemas-microsoft-com:asm.v2">
		<security>
			<requestedPrivileges xmlns="urn:schemas-microsoft-com:asm.v3">
				<requestedExecutionLevel level="asInvoker" uiAccess="false" />
			</requestedPrivileges>
		</security>
	</trustInfo>)"
	R"(</assembly>)"
);

constexpr std::string_view trust_info_multiple_elements_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(<trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
		<security>
			<requestedPrivileges>
				<requestedExecutionLevel level="asInvoker" uiAccess="false" />
			</requestedPrivileges>
			<requestedPrivileges/>
		</security>
	</trustInfo>)"
	R"(</assembly>)"
);

constexpr std::string_view trust_info_absent_attributes_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(<trustInfo xmlns="urn:schemas-microsoft-com:asm.v2">
		<security>
			<requestedPrivileges>
				<requestedExecutionLevel />
			</requestedPrivileges>
		</security>
	</trustInfo>)"
	R"(</assembly>)"
);

constexpr std::string_view dependencies_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(<dependency>
			<dependentAssembly>
				<assemblyIdentity name="dep1" version="v1"
					processorArchitecture="amd64" language="*" publicKeyToken="token1" type="win32" />
			</dependentAssembly>
		</dependency>
		<dependency>
			<dependentAssembly>
				<assemblyIdentity name="dep2" version="v2"
					processorArchitecture="x86" language="en-us" publicKeyToken="token2" type="win32" />
			</dependentAssembly>
			<dependentAssembly>
				<assemblyIdentity name="dep3" version="v3"
					processorArchitecture="x86" language="en-us" publicKeyToken="token3" type="win32" />
			</dependentAssembly>
		</dependency>)"
	R"(</assembly>)"
);

constexpr std::string_view invalid_dependencies_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(<dependency>
			<dependentAssembly>
				<something/>
				<assemblyIdentity name="dep1" version="v1"
					processorArchitecture="amd64" language="*" publicKeyToken="token1" type="win32" />
			</dependentAssembly>
		</dependency>
		<dependency />
		<dependency>
			<dependentAssembly/>
		</dependency>
		<dependency>
			<dependentAssembly>
				<assemblyIdentity name="dep2" version="v2"
					processorArchitecture="x86" language="en-us" publicKeyToken="token2" type="win32" />
				<assemblyIdentity name="dep3" version="v3"
					processorArchitecture="x86" language="en-us" publicKeyToken="token3" type="win32" />
			</dependentAssembly>
			<dependentAssembly/>
		</dependency>)"
	R"(</assembly>)"
);

constexpr std::string_view com_external_proxy_stubs_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(<comInterfaceExternalProxyStub name="name1" iid="iid1" tlbid="tlbid1" proxyStubClsid32="clsid1"
			baseInterface="base1" numMethods="123" />
		<comInterfaceExternalProxyStub name="name2" iid="iid2" tlbid="tlbid2" proxyStubClsid32="clsid2" />
		<comInterfaceExternalProxyStub />")
	R"(</assembly>)"
);

constexpr std::string_view files_manifest(
	R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
	R"(<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">)"
	R"(<assemblyIdentity name="assemblyName" version="1.2.3.4")"
	R"( processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" type="win32" language="*" />)"
	R"(
		<file name="file1" hashalg="sha1" hash="abc">
			<comClass description="desc1" clsid="clsid1" threadingModel="model1"
				tlbid="tlbid1" progid="progid1" miscStatus="miscStatus1"
				miscStatusIcon="miscStatusIcon1" miscStatusContent="miscStatusContent1"
				miscStatusDocprint="miscStatusDocprint1" miscStatusThumbnail="miscStatusThumbnail1">
				<progid>nested_progid1a</progid>
				<progid>nested_progid1b</progid>
			</comClass>
			<typelib tlbid="tlbid1" version="v1" helpdir="helpdir1" resourceid="resourceid1" flags="flags1" />
			<comInterfaceProxyStub name="name1" iid="iid1" tlbid="tlbid1" proxyStubClsid32="clsid1"
				baseInterface="base1" numMethods="123" threadingModel="model1" />
			<windowClass versioned="no">window1</windowClass>

			<comClass description="desc2" clsid="clsid2" threadingModel="model2"
				tlbid="tlbid2" progid="progid2" miscStatus="miscStatus2"
				miscStatusIcon="miscStatusIcon2" miscStatusContent="miscStatusContent2"
				miscStatusDocprint="miscStatusDocprint2" miscStatusThumbnail="miscStatusThumbnail2" />
			<typelib tlbid="tlbid2" version="v2" helpdir="helpdir2" resourceid="resourceid2" flags="flags2" />
			<comInterfaceProxyStub name="name2" iid="iid2" tlbid="tlbid2" proxyStubClsid32="clsid2"
				baseInterface="base2" numMethods="456" threadingModel="model2" />
			<windowClass versioned="yes">window2</windowClass>
		</file>
		<file>
			<comClass>
				<progid>   </progid>
			</comClass>
			<typelib />
			<comInterfaceProxyStub />
			<windowClass />
		</file>
)"
	R"(</assembly>)"
);

buffers::input_buffer_ptr buf_from_string(std::string_view str)
{
	const auto* ptr = reinterpret_cast<const std::byte*>(str.data());
	return std::make_shared<buffers::input_memory_buffer>(ptr, str.size());
}

class ManifestTestFixture : public ::testing::Test
{
public:
	void load(std::string_view xml)
	{
		accessor_ = pugixml::parse_manifest(buf_from_string(xml));
		manifest = parse_manifest(*accessor_);
	}

	void expect_empty_assembly_identity() const
	{
		EXPECT_FALSE(manifest.get_assembly_identity().get_language_raw());
		EXPECT_FALSE(manifest.get_assembly_identity().get_processor_architecture_raw());
		EXPECT_FALSE(manifest.get_assembly_identity().get_public_key_token_raw());
		EXPECT_TRUE(manifest.get_assembly_identity().get_type_raw().empty());
		EXPECT_TRUE(manifest.get_assembly_identity().get_version_raw().empty());
	}

	template<auto Getter, auto InvalidValueErrc, auto MultipleValuesErrc>
	void test_bool_flags(std::string_view tag_name)
	{
		test_bool_flag<Getter, InvalidValueErrc>(tag_name, false);
		test_bool_flag<Getter, InvalidValueErrc>(tag_name, true);
		test_multiple_bool_flags<Getter, MultipleValuesErrc>(tag_name);
	}

	static std::string format_window_settings_manifest_xml_multiple_flags(
		std::string_view tag_name)
	{
		std::string manifest_xml(windows_settings_manifest_header);
		manifest_xml += '<';
		manifest_xml += tag_name;
		manifest_xml += ">true</";
		manifest_xml += tag_name;
		manifest_xml += "><";
		manifest_xml += tag_name;
		manifest_xml += ">true</";
		manifest_xml += tag_name;
		manifest_xml += '>';
		manifest_xml += windows_settings_manifest_footer;
		return manifest_xml;
	}
	
	template<auto Getter, auto MultipleValuesErrc>
	void test_multiple_bool_flags(std::string_view tag_name)
	{
		load(format_window_settings_manifest_xml_multiple_flags(tag_name));
		expect_contains_errors(manifest, MultipleValuesErrc);
		EXPECT_FALSE((manifest.*Getter)()) << tag_name;
	}

	static std::string format_window_settings_manifest_xml(std::string_view tag_name,
		std::string_view value)
	{
		std::string manifest_xml(windows_settings_manifest_header);
		manifest_xml += '<';
		manifest_xml += tag_name;
		manifest_xml += '>';
		manifest_xml += value;
		manifest_xml += "</";
		manifest_xml += tag_name;
		manifest_xml += '>';
		manifest_xml += windows_settings_manifest_footer;
		return manifest_xml;
	}

	template<auto Getter, auto InvalidValueErrc>
	void test_bool_flag(std::string_view tag_name, std::optional<bool> value)
	{
		auto manifest_xml = format_window_settings_manifest_xml(tag_name,
			value ? (*value ? "true" : "false") : "xyz");

		load(manifest_xml);

		if (value)
		{
			expect_contains_errors(manifest);
			EXPECT_EQ((manifest.*Getter)(), *value) << tag_name;
		}
		else
		{
			expect_contains_errors(manifest, InvalidValueErrc);
			EXPECT_FALSE((manifest.*Getter)()) << tag_name;
		}
	}

	void expect_empty(bool has_assembly_identity = false) const
	{
		EXPECT_EQ(manifest.no_inherit(), assembly_no_inherit::absent);

		if (!has_assembly_identity)
		{
			EXPECT_TRUE(manifest.get_manifest_version().empty());
			expect_empty_assembly_identity();
		}

		EXPECT_FALSE(manifest.get_supported_os_list());
		EXPECT_TRUE(manifest.get_dependencies().empty());
		EXPECT_TRUE(manifest.get_files().empty());
		EXPECT_FALSE(manifest.get_active_code_page());
		EXPECT_FALSE(manifest.get_description());
		EXPECT_FALSE(manifest.get_dpi_awareness().get_dpi_awareness_raw());
		EXPECT_FALSE(manifest.get_dpi_awareness().get_dpi_aware_raw());

		EXPECT_FALSE(manifest.auto_elevate());
		EXPECT_FALSE(manifest.disable_theming());
		EXPECT_FALSE(manifest.disable_window_filtering());
		EXPECT_FALSE(manifest.gdi_scaling());
		EXPECT_FALSE(manifest.high_resolution_scrolling_aware());
		EXPECT_FALSE(manifest.long_path_aware());
		EXPECT_FALSE(manifest.printer_driver_isolation());
		EXPECT_FALSE(manifest.ultra_high_resolution_scrolling_aware());
		EXPECT_FALSE(manifest.get_heap_type());
		EXPECT_FALSE(manifest.get_requested_privileges());
		EXPECT_FALSE(manifest.get_msix_identity());
		EXPECT_TRUE(manifest.get_com_interface_external_proxy_stubs().empty());
	}

	template<auto... Errc>
	void verify_assembly_identity() const
	{
		expect_contains_errors(manifest);
		expect_empty(true);
		expect_contains_errors(manifest.get_assembly_identity(), Errc...);

		EXPECT_EQ(manifest.get_manifest_version(), "1.0");
		EXPECT_EQ(manifest.get_assembly_identity().get_language_raw(), "*");
		EXPECT_EQ(manifest.get_assembly_identity().get_processor_architecture_raw(), "amd64");
		EXPECT_EQ(manifest.get_assembly_identity().get_public_key_token_raw(),
			"6595b64144ccf1df");
		EXPECT_EQ(manifest.get_assembly_identity().get_type_raw(), "win32");
		EXPECT_EQ(manifest.get_assembly_identity().get_version_raw(), "1.2.3.4");
	}

	void verify_dependencies() const
	{
		EXPECT_EQ(manifest.get_dependencies()[0].get_language_raw(), "*");
		EXPECT_EQ(manifest.get_dependencies()[0].get_processor_architecture_raw(), "amd64");
		EXPECT_EQ(manifest.get_dependencies()[0].get_public_key_token_raw(), "token1");
		EXPECT_EQ(manifest.get_dependencies()[0].get_type_raw(), "win32");
		EXPECT_EQ(manifest.get_dependencies()[0].get_version_raw(), "v1");
		EXPECT_EQ(manifest.get_dependencies()[0].get_name(), "dep1");
		EXPECT_EQ(manifest.get_dependencies()[1].get_language_raw(), "en-us");
		EXPECT_EQ(manifest.get_dependencies()[1].get_processor_architecture_raw(), "x86");
		EXPECT_EQ(manifest.get_dependencies()[1].get_public_key_token_raw(), "token2");
		EXPECT_EQ(manifest.get_dependencies()[1].get_type_raw(), "win32");
		EXPECT_EQ(manifest.get_dependencies()[1].get_version_raw(), "v2");
		EXPECT_EQ(manifest.get_dependencies()[1].get_name(), "dep2");
	}

public:
	native_manifest_details manifest;

private:
	manifest_accessor_interface_ptr accessor_;
};
} //namespace

TEST_F(ManifestTestFixture, Invalid)
{
	load("<");
	expect_contains_errors(manifest, manifest_errc::empty_manifest);
	expect_empty();
}

TEST_F(ManifestTestFixture, Empty)
{
	load(empty_manifest);
	expect_contains_errors(manifest, manifest_errc::invalid_manifest_version,
		manifest_errc::absent_manifest_version,
		manifest_errc::absent_assembly_identity);
	expect_empty();
}

TEST_F(ManifestTestFixture, AssemblyIdentity)
{
	load(assembly_identity_manifest);
	verify_assembly_identity();
}

TEST_F(ManifestTestFixture, EmptyAssemblyIdentity)
{
	load(empty_assembly_identity_manifest);
	expect_contains_errors(manifest);
	expect_empty(true);
	expect_contains_errors(manifest.get_assembly_identity(),
		manifest_errc::absent_assembly_identity_type,
		manifest_errc::absent_assembly_identity_name,
		manifest_errc::absent_assembly_identity_version);

	EXPECT_EQ(manifest.get_manifest_version(), "1.0");
	expect_empty_assembly_identity();
}

TEST_F(ManifestTestFixture, WindowsSettings)
{
	test_bool_flags<&native_manifest_details::auto_elevate,
		manifest_errc::invalid_auto_elevate_elem,
		manifest_errc::multiple_auto_elevate_elem>("ws05:autoElevate");
	test_bool_flags<&native_manifest_details::disable_theming,
		manifest_errc::invalid_disable_theming_elem,
		manifest_errc::multiple_disable_theming_elem>("ws05:disableTheming");
	test_bool_flags<&native_manifest_details::disable_window_filtering,
		manifest_errc::invalid_disable_window_filtering_elem,
		manifest_errc::multiple_disable_window_filtering_elem>(
			"ws11:disableWindowFiltering");
	test_bool_flags<&native_manifest_details::gdi_scaling,
		manifest_errc::invalid_gdi_scaling_elem,
		manifest_errc::multiple_gdi_scaling_elem>("ws17:gdiScaling");
	test_bool_flags<&native_manifest_details::high_resolution_scrolling_aware,
		manifest_errc::invalid_high_resolution_scrolling_aware_elem,
		manifest_errc::multiple_high_resolution_scrolling_aware_elem>(
			"ws13:highResolutionScrollingAware");
	test_bool_flags<&native_manifest_details::long_path_aware,
		manifest_errc::invalid_long_path_aware_elem,
		manifest_errc::multiple_long_path_aware_elem>(
			"ws16:longPathAware");
	test_bool_flags<&native_manifest_details::printer_driver_isolation,
		manifest_errc::invalid_printer_driver_isolation_elem,
		manifest_errc::multiple_printer_driver_isolation_elem>(
			"ws11:printerDriverIsolation");
	test_bool_flags<&native_manifest_details::ultra_high_resolution_scrolling_aware,
		manifest_errc::invalid_ultra_high_resolution_scrolling_aware_elem,
		manifest_errc::multiple_ultra_high_resolution_scrolling_aware_elem>(
			"ws13:ultraHighResolutionScrollingAware");
}

TEST_F(ManifestTestFixture, Description)
{
	load(description_manifest);
	expect_contains_errors(manifest);
	EXPECT_EQ(manifest.get_description(), "Description line 1\nDescription line 2");
}

TEST_F(ManifestTestFixture, WindowsSettingsHeapType)
{
	using get_heap_type_func = const std::optional<heap_type>& (
		native_manifest_details::*)() const& noexcept;
	test_multiple_bool_flags<static_cast<get_heap_type_func>(&native_manifest_details::get_heap_type),
		manifest_errc::multiple_heap_type_elements>("ws20:heapType");

	load(format_window_settings_manifest_xml("ws20:heapType", "xxx"));
	ASSERT_TRUE(manifest.get_heap_type());
	EXPECT_EQ(manifest.get_heap_type()->get_heap_type_raw(), "xxx");
	expect_contains_errors(manifest);
}

TEST_F(ManifestTestFixture, WindowsSettingsActiveCodePage)
{
	using get_active_code_page_func = const std::optional<active_code_page>& (
		native_manifest_details::*)() const& noexcept;
	test_multiple_bool_flags<static_cast<get_active_code_page_func>(
		&native_manifest_details::get_active_code_page),
		manifest_errc::multiple_active_code_page_elements>("ws19:activeCodePage");

	load(format_window_settings_manifest_xml("ws19:activeCodePage", "utf-8"));
	ASSERT_TRUE(manifest.get_active_code_page());
	EXPECT_EQ(manifest.get_active_code_page()->get_name(), "utf-8");
	expect_contains_errors(manifest);
}

TEST_F(ManifestTestFixture, WindowsSettingsDpiAware)
{
	load(format_window_settings_manifest_xml_multiple_flags("ws05:dpiAware"));
	ASSERT_FALSE(manifest.get_dpi_awareness().get_dpi_aware_raw());
	expect_contains_errors(manifest, manifest_errc::multiple_dpi_aware_elements);

	load(format_window_settings_manifest_xml("ws05:dpiAware", "aware"));
	EXPECT_EQ(manifest.get_dpi_awareness().get_dpi_aware_raw(), "aware");
	expect_contains_errors(manifest);
}

TEST_F(ManifestTestFixture, WindowsSettingsDpiAwareness)
{
	load(format_window_settings_manifest_xml_multiple_flags("ws16:dpiAwareness"));
	ASSERT_FALSE(manifest.get_dpi_awareness().get_dpi_awareness_raw());
	expect_contains_errors(manifest, manifest_errc::multiple_dpi_awareness_elements);

	load(format_window_settings_manifest_xml("ws16:dpiAwareness", "aware"));
	EXPECT_EQ(manifest.get_dpi_awareness().get_dpi_awareness_raw(), "aware");
	expect_contains_errors(manifest);
}

TEST_F(ManifestTestFixture, MsixIdentity)
{
	load(msix_identity_manifest);
	expect_contains_errors(manifest);
	ASSERT_TRUE(manifest.get_msix_identity());
	expect_contains_errors(*manifest.get_msix_identity());

	EXPECT_EQ(manifest.get_msix_identity()->get_publisher(), "Publisher");
	EXPECT_EQ(manifest.get_msix_identity()->get_application_id(), "Application");
	EXPECT_EQ(manifest.get_msix_identity()->get_package_name(), "Package");
}

TEST_F(ManifestTestFixture, EmptyMsixIdentity)
{
	load(empty_msix_identity_manifest);
	expect_contains_errors(manifest);
	ASSERT_TRUE(manifest.get_msix_identity());
	expect_contains_errors(*manifest.get_msix_identity(),
		manifest_errc::absent_msix_application_id,
		manifest_errc::absent_msix_package_name,
		manifest_errc::absent_msix_publisher);

	EXPECT_EQ(manifest.get_msix_identity()->get_publisher(), "");
	EXPECT_EQ(manifest.get_msix_identity()->get_application_id(), "");
	EXPECT_EQ(manifest.get_msix_identity()->get_package_name(), "");
}

TEST_F(ManifestTestFixture, MultipleMsixIdentities)
{
	load(multiple_msix_identity_manifest);
	expect_contains_errors(manifest,
		manifest_errc::multiple_msix_elements);
	EXPECT_FALSE(manifest.get_msix_identity());
}

TEST_F(ManifestTestFixture, MultipleWindowsSettingsElements)
{
	load(multiple_windows_settings_manifest);
	expect_contains_errors(manifest,
		manifest_errc::multiple_windows_settings_elements);
}

TEST_F(ManifestTestFixture, AssemblyIdentityWrongIndex)
{
	load(assembly_identity_wrong_index_manifest);
	verify_assembly_identity<manifest_errc::invalid_assembly_identity_element_position>();
}

TEST_F(ManifestTestFixture, NoInherit)
{
	load(no_inherit_manifest);
	expect_contains_errors(manifest);
	expect_contains_errors(manifest.get_assembly_identity());
	EXPECT_EQ(manifest.no_inherit(), assembly_no_inherit::no_inherit);
}

TEST_F(ManifestTestFixture, NoInheritable)
{
	load(no_inheritable_manifest);
	expect_contains_errors(manifest);
	expect_contains_errors(manifest.get_assembly_identity());
	EXPECT_EQ(manifest.no_inherit(), assembly_no_inherit::no_inheritable);
}

TEST_F(ManifestTestFixture, MultipleNoInherit)
{
	load(multiple_no_inherit_manifest);
	expect_contains_errors(manifest, manifest_errc::multiple_no_inherit_elements);
	expect_contains_errors(manifest.get_assembly_identity(),
		manifest_errc::invalid_assembly_identity_element_position);
	EXPECT_EQ(manifest.no_inherit(), assembly_no_inherit::absent);
}

TEST_F(ManifestTestFixture, WrongNoInheritPos)
{
	load(wrong_no_inherit_pos_manifest);
	expect_contains_errors(manifest, manifest_errc::no_inherit_element_not_first);
	expect_contains_errors(manifest.get_assembly_identity());
	EXPECT_EQ(manifest.no_inherit(), assembly_no_inherit::no_inherit);
}

TEST_F(ManifestTestFixture, NoInheritWrongAssemblyIdentityPos)
{
	load(no_inherit_wrong_assembly_identity_pos_manifest);
	expect_contains_errors(manifest);
	expect_contains_errors(manifest.get_assembly_identity(),
		manifest_errc::invalid_assembly_identity_element_position);
	EXPECT_EQ(manifest.no_inherit(), assembly_no_inherit::no_inherit);
}

TEST_F(ManifestTestFixture, SupportedOs)
{
	load(supported_os_manifest);
	expect_contains_errors(manifest);
	ASSERT_TRUE(manifest.get_supported_os_list());
	expect_contains_errors(*manifest.get_supported_os_list());
	EXPECT_EQ(manifest.get_supported_os_list()->get_max_tested_os_version_raw(),
		"10.0.19041.0");
	EXPECT_THAT(manifest.get_supported_os_list()->get_list_raw(),
		testing::UnorderedElementsAre("{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}",
			"{1f676c76-80e1-4239-95bb-83d0f6d0da78}",
			"{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}",
			"{35138b9a-5d96-4fbd-8e2d-a2440225f93a}",
			"{e2011457-1546-43c5-a5fe-008deee3d3f0}"));
}

TEST_F(ManifestTestFixture, SupportedOsMultipleCompatibility)
{
	load(supported_os_manifest_multiple_compatibility);
	expect_contains_errors(manifest,
		manifest_errc::multiple_compatibility_elements);
	EXPECT_FALSE(manifest.get_supported_os_list());
}

TEST_F(ManifestTestFixture, TrustInfo)
{
	load(trust_info_manifest);
	expect_contains_errors(manifest);
	ASSERT_TRUE(manifest.get_requested_privileges());
	EXPECT_EQ(manifest.get_requested_privileges()->get_level_raw(), "asInvoker");
	EXPECT_EQ(manifest.get_requested_privileges()->get_ui_access_raw(), "false");
}

TEST_F(ManifestTestFixture, TrustInfoMultipleElements)
{
	load(trust_info_multiple_elements_manifest);
	expect_contains_errors(manifest,
		manifest_errc::multiple_requested_privileges_elements);
	EXPECT_FALSE(manifest.get_requested_privileges());
}

TEST_F(ManifestTestFixture, TrustInfoAbsentAttributes)
{
	load(trust_info_absent_attributes_manifest);
	expect_contains_errors(manifest,
		manifest_errc::absent_requested_execution_ui_access,
		manifest_errc::absent_requested_execution_level);
	ASSERT_TRUE(manifest.get_requested_privileges());
	EXPECT_TRUE(manifest.get_requested_privileges()->get_level_raw().empty());
	EXPECT_TRUE(manifest.get_requested_privileges()->get_ui_access_raw().empty());
}

TEST_F(ManifestTestFixture, Dependencies)
{
	load(dependencies_manifest);
	expect_contains_errors(manifest);
	ASSERT_EQ(manifest.get_dependencies().size(), 3u);
	expect_contains_errors(manifest.get_dependencies()[0]);
	expect_contains_errors(manifest.get_dependencies()[1]);
	expect_contains_errors(manifest.get_dependencies()[2]);
	verify_dependencies();
	EXPECT_EQ(manifest.get_dependencies()[2].get_language_raw(), "en-us");
	EXPECT_EQ(manifest.get_dependencies()[2].get_processor_architecture_raw(), "x86");
	EXPECT_EQ(manifest.get_dependencies()[2].get_public_key_token_raw(), "token3");
	EXPECT_EQ(manifest.get_dependencies()[2].get_type_raw(), "win32");
	EXPECT_EQ(manifest.get_dependencies()[2].get_version_raw(), "v3");
	EXPECT_EQ(manifest.get_dependencies()[2].get_name(), "dep3");
}

TEST_F(ManifestTestFixture, InvalidDependencies)
{
	load(invalid_dependencies_manifest);
	expect_contains_errors(manifest, manifest_errc::invalid_dependencies);
	ASSERT_EQ(manifest.get_dependencies().size(), 2u);
	expect_contains_errors(manifest.get_dependencies()[0],
		manifest_errc::invalid_dependend_assembly_element_position);
	expect_contains_errors(manifest.get_dependencies()[1],
		manifest_errc::multiple_assembly_identities);
	verify_dependencies();
}

TEST_F(ManifestTestFixture, ComInterfaceExternalProxyStub)
{
	load(com_external_proxy_stubs_manifest);
	expect_contains_errors(manifest);
	ASSERT_EQ(manifest.get_com_interface_external_proxy_stubs().size(), 3u);
	const auto& stub0 = manifest.get_com_interface_external_proxy_stubs()[0];
	const auto& stub1 = manifest.get_com_interface_external_proxy_stubs()[1];
	const auto& stub2 = manifest.get_com_interface_external_proxy_stubs()[2];
	expect_contains_errors(stub0);
	expect_contains_errors(stub1);
	expect_contains_errors(stub2, manifest_errc::absent_iid);

	EXPECT_EQ(stub0.get_base_interface_raw(), "base1");
	EXPECT_EQ(stub0.get_iid_raw(), "iid1");
	EXPECT_EQ(stub0.get_tlbid_raw(), "tlbid1");
	EXPECT_EQ(stub0.get_proxy_stub_clsid32_raw(), "clsid1");
	EXPECT_EQ(stub0.get_num_methods_raw(), "123");
	EXPECT_EQ(stub0.get_name(), "name1");

	EXPECT_FALSE(stub1.get_base_interface_raw());
	EXPECT_EQ(stub1.get_iid_raw(), "iid2");
	EXPECT_EQ(stub1.get_tlbid_raw(), "tlbid2");
	EXPECT_EQ(stub1.get_proxy_stub_clsid32_raw(), "clsid2");
	EXPECT_FALSE(stub1.get_num_methods_raw());
	EXPECT_EQ(stub1.get_name(), "name2");

	EXPECT_FALSE(stub2.get_base_interface_raw());
	EXPECT_TRUE(stub2.get_iid_raw().empty());
	EXPECT_FALSE(stub2.get_tlbid_raw());
	EXPECT_FALSE(stub2.get_proxy_stub_clsid32_raw());
	EXPECT_FALSE(stub2.get_num_methods_raw());
	EXPECT_FALSE(stub2.get_name());
}

TEST_F(ManifestTestFixture, File)
{
	load(files_manifest);
	expect_contains_errors(manifest);
	ASSERT_EQ(manifest.get_files().size(), 2u);
	auto& file1 = manifest.get_files()[0];
	auto& file2 = manifest.get_files()[1];

	{
		expect_contains_errors(file1);
		EXPECT_EQ(file1.get_name(), "file1");
		EXPECT_EQ(file1.get_hash_algorithm_raw(), "sha1");
		EXPECT_EQ(file1.get_hash_raw(), "abc");
		ASSERT_EQ(file1.get_com_classes().size(), 2u);
		auto& class1 = file1.get_com_classes()[0];
		auto& class2 = file1.get_com_classes()[1];
		expect_contains_errors(class1);
		expect_contains_errors(class2);
		EXPECT_EQ(class1.get_description(), "desc1");
		EXPECT_EQ(class1.get_clsid_raw(), "clsid1");
		EXPECT_EQ(class1.get_threading_model_raw(), "model1");
		EXPECT_EQ(class1.get_tlbid_raw(), "tlbid1");
		EXPECT_EQ(class1.get_progid_raw(), "progid1");
		EXPECT_EQ(class1.get_misc_status_raw(), "miscStatus1");
		EXPECT_EQ(class1.get_misc_status_icon_raw(), "miscStatusIcon1");
		EXPECT_EQ(class1.get_misc_status_content_raw(), "miscStatusContent1");
		EXPECT_EQ(class1.get_misc_status_docprint_raw(), "miscStatusDocprint1");
		EXPECT_EQ(class1.get_misc_status_thumbnail_raw(), "miscStatusThumbnail1");
		EXPECT_THAT(class1.get_additional_progids(), testing::UnorderedElementsAre(
			"nested_progid1a", "nested_progid1b"
		));
		EXPECT_EQ(class2.get_description(), "desc2");
		EXPECT_EQ(class2.get_clsid_raw(), "clsid2");
		EXPECT_EQ(class2.get_threading_model_raw(), "model2");
		EXPECT_EQ(class2.get_tlbid_raw(), "tlbid2");
		EXPECT_EQ(class2.get_progid_raw(), "progid2");
		EXPECT_EQ(class2.get_misc_status_raw(), "miscStatus2");
		EXPECT_EQ(class2.get_misc_status_icon_raw(), "miscStatusIcon2");
		EXPECT_EQ(class2.get_misc_status_content_raw(), "miscStatusContent2");
		EXPECT_EQ(class2.get_misc_status_docprint_raw(), "miscStatusDocprint2");
		EXPECT_EQ(class2.get_misc_status_thumbnail_raw(), "miscStatusThumbnail2");
		EXPECT_TRUE(class2.get_additional_progids().empty());
		ASSERT_EQ(file1.get_com_typelibs().size(), 2u);
		auto& typelib1 = file1.get_com_typelibs()[0];
		auto& typelib2 = file1.get_com_typelibs()[1];
		expect_contains_errors(typelib1);
		expect_contains_errors(typelib2);
		EXPECT_EQ(typelib1.get_tlbid_raw(), "tlbid1");
		EXPECT_EQ(typelib1.get_version_raw(), "v1");
		EXPECT_EQ(typelib1.get_help_dir(), "helpdir1");
		EXPECT_EQ(typelib1.get_resource_id_raw(), "resourceid1");
		EXPECT_EQ(typelib1.get_flags_raw(), "flags1");
		EXPECT_EQ(typelib2.get_tlbid_raw(), "tlbid2");
		EXPECT_EQ(typelib2.get_version_raw(), "v2");
		EXPECT_EQ(typelib2.get_help_dir(), "helpdir2");
		EXPECT_EQ(typelib2.get_resource_id_raw(), "resourceid2");
		EXPECT_EQ(typelib2.get_flags_raw(), "flags2");
		ASSERT_EQ(file1.get_window_classes().size(), 2u);
		auto& window1 = file1.get_window_classes()[0];
		auto& window2 = file1.get_window_classes()[1];
		expect_contains_errors(window1);
		expect_contains_errors(window2);
		EXPECT_EQ(window1.get_class(), "window1");
		EXPECT_EQ(window1.is_versioned_raw(), "no");
		EXPECT_EQ(window2.get_class(), "window2");
		EXPECT_EQ(window2.is_versioned_raw(), "yes");
		ASSERT_EQ(file1.get_com_interface_proxy_stubs().size(), 2u);
		auto& stub1 = file1.get_com_interface_proxy_stubs()[0];
		auto& stub2 = file1.get_com_interface_proxy_stubs()[1];
		expect_contains_errors(stub1);
		expect_contains_errors(stub2);
		EXPECT_EQ(stub1.get_name(), "name1");
		EXPECT_EQ(stub1.get_iid_raw(), "iid1");
		EXPECT_EQ(stub1.get_tlbid_raw(), "tlbid1");
		EXPECT_EQ(stub1.get_proxy_stub_clsid32_raw(), "clsid1");
		EXPECT_EQ(stub1.get_base_interface_raw(), "base1");
		EXPECT_EQ(stub1.get_num_methods_raw(), "123");
		EXPECT_EQ(stub1.get_threading_model_raw(), "model1");
		EXPECT_EQ(stub2.get_name(), "name2");
		EXPECT_EQ(stub2.get_iid_raw(), "iid2");
		EXPECT_EQ(stub2.get_tlbid_raw(), "tlbid2");
		EXPECT_EQ(stub2.get_proxy_stub_clsid32_raw(), "clsid2");
		EXPECT_EQ(stub2.get_base_interface_raw(), "base2");
		EXPECT_EQ(stub2.get_num_methods_raw(), "456");
		EXPECT_EQ(stub2.get_threading_model_raw(), "model2");
	}

	{
		expect_contains_errors(file2, manifest_errc::absent_file_name);
		EXPECT_TRUE(file2.get_name().empty());
		EXPECT_FALSE(file2.get_hash_algorithm_raw());
		EXPECT_FALSE(file2.get_hash_raw());
		ASSERT_EQ(file2.get_com_classes().size(), 1u);
		auto& class1 = file2.get_com_classes()[0];
		expect_contains_errors(class1, manifest_errc::absent_clsid,
			manifest_errc::empty_progids);
		EXPECT_FALSE(class1.get_description());
		EXPECT_TRUE(class1.get_clsid_raw().empty());
		EXPECT_FALSE(class1.get_threading_model_raw());
		EXPECT_FALSE(class1.get_tlbid_raw());
		EXPECT_FALSE(class1.get_progid_raw());
		EXPECT_FALSE(class1.get_misc_status_raw());
		EXPECT_FALSE(class1.get_misc_status_icon_raw());
		EXPECT_FALSE(class1.get_misc_status_content_raw());
		EXPECT_FALSE(class1.get_misc_status_docprint_raw());
		EXPECT_FALSE(class1.get_misc_status_thumbnail_raw());
		EXPECT_THAT(class1.get_additional_progids(), testing::UnorderedElementsAre(""));
		ASSERT_EQ(file2.get_com_typelibs().size(), 1u);
		auto& typelib1 = file2.get_com_typelibs()[0];
		expect_contains_errors(typelib1, manifest_errc::absent_helpdir,
			manifest_errc::absent_tlbid, manifest_errc::absent_version);
		EXPECT_TRUE(typelib1.get_tlbid_raw().empty());
		EXPECT_TRUE(typelib1.get_version_raw().empty());
		EXPECT_TRUE(typelib1.get_help_dir().empty());
		EXPECT_FALSE(typelib1.get_resource_id_raw());
		EXPECT_FALSE(typelib1.get_flags_raw());
		ASSERT_EQ(file2.get_window_classes().size(), 1u);
		auto& window1 = file2.get_window_classes()[0];
		expect_contains_errors(window1, manifest_errc::empty_window_class);
		EXPECT_TRUE(window1.get_class().empty());
		EXPECT_FALSE(window1.is_versioned_raw());
		ASSERT_EQ(file2.get_com_interface_proxy_stubs().size(), 1u);
		auto& stub1 = file2.get_com_interface_proxy_stubs()[0];
		expect_contains_errors(stub1, manifest_errc::absent_iid);
		EXPECT_FALSE(stub1.get_name());
		EXPECT_TRUE(stub1.get_iid_raw().empty());
		EXPECT_FALSE(stub1.get_tlbid_raw());
		EXPECT_FALSE(stub1.get_proxy_stub_clsid32_raw());
		EXPECT_FALSE(stub1.get_base_interface_raw());
		EXPECT_FALSE(stub1.get_num_methods_raw());
		EXPECT_FALSE(stub1.get_threading_model_raw());
	}
}
