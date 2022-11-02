#include "pe_bliss2/resources/manifest.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/resources/manifest_accessor_interface.h"
#include "utilities/string.h"

namespace
{
struct manifest_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "manifest";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::resources::manifest_errc;
		switch (static_cast<pe_bliss::resources::manifest_errc>(ev))
		{
		case invalid_manifest_version:
			return "Invalid manifest version";
		case invalid_version_string:
			return "Invalid version string format";
		case invalid_hex_string:
			return "Invalid HEX string format";
		case invalid_com_progid:
			return "Invalid COM PROGID";
		case invalid_lcid_string:
			return "Invalid LCID string formar";
		case invalid_bool_string:
			return "Invalid BOOL string format";
		case empty_manifest:
			return "Empty manifest";
		case absent_manifest_version:
			return "Absent manifest version";
		case no_inherit_element_not_first:
			return "noInherit/noInheritable element is not the first one";
		case absent_assembly_identity:
			return "Absent assemblyIdentity element";
		case invalid_assembly_identity_element_position:
			return "Invalid assemblyIdentity element position";
		case multiple_assembly_identities:
			return "Multiple assemblyIdentity elements";
		case multiple_no_inherit_elements:
			return "multiple noInherit/noInheritable elements";
		case absent_assembly_identity_type:
			return "Absent assemblyIdentity type attribute";
		case absent_assembly_identity_name:
			return "Absent assemblyIdentity name attribute";
		case absent_assembly_identity_version:
			return "Absent assemblyIdentity version attribute";
		case multiple_compatibility_elements:
			return "Multiple compatibility elements";
		case multiple_maxversiontested_elements:
			return "Multiple maxversiontested elements";
		case invalid_dependencies:
			return "Invalid dependency elements format";
		case invalid_dependend_assembly_element_position:
			return "Invalid dependentAssembly element position";
		case invalid_file_size_string:
			return "Invalid file size string";
		case absent_file_name:
			return "Absent file name";
		case absent_clsid:
			return "Absent CLSID";
		case absent_tlbid:
			return "Absent TLBID";
		case absent_version:
			return "Absent version";
		case absent_helpdir:
			return "Absent help directory";
		case absent_iid:
			return "Absent IID";
		case invalid_num_methods_string:
			return "Invalid numMethods value";
		case multiple_application_elements:
			return "Multiple application elements";
		case multiple_windows_settings_elements:
			return "Multiple windowsSettings elements";
		case multiple_active_code_page_elements:
			return "Multiple activeCodePage elements";
		case multiple_auto_elevate_elem:
			return "Multiple autoElevate elements";
		case invalid_auto_elevate_elem:
			return "Invalid autoElevate element value";
		case multiple_disable_theming_elem:
			return "Multiple disableTheming elements";
		case invalid_disable_theming_elem:
			return "Invalid disableTheming element value";
		case multiple_disable_window_filtering_elem:
			return "Multiple disableWindowFiltering elements";
		case invalid_disable_window_filtering_elem:
			return "Invalid disableWindowFiltering element value";
		case multiple_gdi_scaling_elem:
			return "Multiple gdiScaling elements";
		case invalid_gdi_scaling_elem:
			return "Invalid gdiScaling element value";
		case multiple_high_resolution_scrolling_aware_elem:
			return "Multiple highResolutionScrollingAware elements";
		case invalid_high_resolution_scrolling_aware_elem:
			return "Invalid highResolutionScrollingAware element value";
		case multiple_ultra_high_resolution_scrolling_aware_elem:
			return "Multiple ultraHighResolutionScrollingAware elements";
		case invalid_ultra_high_resolution_scrolling_aware_elem:
			return "Invalid ultraHighResolutionScrollingAware element value";
		case multiple_long_path_aware_elem:
			return "Multiple longPathAware element";
		case invalid_long_path_aware_elem:
			return "Invalid longPathAware element value";
		case multiple_printer_driver_isolation_elem:
			return "Multiple printerDriverIsolation elements";
		case invalid_printer_driver_isolation_elem:
			return "Invalid printerDriverIsolation element value";
		case multiple_dpi_aware_elements:
			return "Multiple dpiAware elements";
		case multiple_dpi_awareness_elements:
			return "Mutliple dpiAwareness elements";
		case multiple_heap_type_elements:
			return "Multiple heapType elements";
		case multiple_trust_info_elements:
			return "Multiple trustInfo elements";
		case multiple_security_elements:
			return "Multiple security elements";
		case multiple_requested_privileges_elements:
			return "Multiple requestedPrivileges elements";
		case multiple_requested_execution_level_elements:
			return "Multiple requestedExecutionLevel elements";
		case absent_requested_execution_level:
			return "Absent requestedExecutionLevel level attribute";
		case absent_requested_execution_ui_access:
			return "Absent requestedExecutionLevel uiAccess attribute";
		case multiple_msix_elements:
			return "Multiple msix elements";
		case absent_msix_publisher:
			return "Absent msix publisher attribute";
		case absent_msix_package_name:
			return "Absent msix packageName attribute";
		case absent_msix_application_id:
			return "Absent msix applicationId attribute";
		case empty_progids:
			return "Empty PROGID(s)";
		case empty_window_class:
			return "Empty window class";
		default:
			return {};
		}
	}
};

const manifest_category manifest_category_instance;

using guid_version_type = std::pair<std::string_view,
	pe_bliss::resources::assembly_supported_os>;

template<std::size_t Parts>
auto parse_version_impl(std::string_view version_string)
{
	using namespace pe_bliss;
	using namespace pe_bliss::resources;
	std::array<std::uint16_t, Parts> version_numbers{};
	static constexpr std::string_view delim{ "." };
	std::size_t number_index{};
	for (const auto& word : std::views::split(version_string, delim)) {
		if (number_index >= version_numbers.size())
			throw pe_error(manifest_errc::invalid_version_string);

		std::string_view number_str(word.begin(), word.end());
		auto res = std::from_chars(number_str.data(),
			number_str.data() + number_str.size(),
			version_numbers[number_index++]);
		if (res.ptr != number_str.data() + number_str.size()
			|| res.ec != std::errc{})
		{
			throw pe_error(manifest_errc::invalid_version_string);
		}
	}
	if (number_index != version_numbers.size())
		throw pe_error(manifest_errc::invalid_version_string);

	return version_numbers;
}
} //namespace

namespace pe_bliss::resources
{

std::error_code make_error_code(manifest_errc e) noexcept
{
	return { static_cast<int>(e), manifest_category_instance };
}

full_version parse_full_version(std::string_view version_string)
{
	auto version_numbers = parse_version_impl<4u>(version_string);
	return {
		.major = version_numbers[0],
		.minor = version_numbers[1],
		.build = version_numbers[2],
		.revision = version_numbers[3]
	};
}

short_version parse_short_version(std::string_view version_string)
{
	auto version_numbers = parse_version_impl<2u>(version_string);
	return {
		.major = version_numbers[0],
		.minor = version_numbers[1]
	};
}

template<typename... Bases>
assembly_identity_type assembly_identity_base<Bases...>
	::get_type() const noexcept
{
	if (type_ == "win32")
		return assembly_identity_type::win32;

	return assembly_identity_type::unknown;
}

template<typename... Bases>
std::optional<lcid_info> assembly_identity_base<Bases...>
	::get_language() const noexcept
{
	if (!language_)
		return {};

	if (*language_ == "*")
		return get_lcid_info(0u); //neutral

	auto language_lower = *language_;
	utilities::to_lower_inplace(language_lower);
	return get_lcid_info(language_lower);
}

template<typename... Bases>
assembly_processor_architecture assembly_identity_base<Bases...>
	::get_processor_architecture() const noexcept
{
	if (!processor_architecture_)
		return assembly_processor_architecture::unspecified;

	if (*processor_architecture_ == "x86")
		return assembly_processor_architecture::x86;
	if (*processor_architecture_ == "amd64")
		return assembly_processor_architecture::amd64;
	if (*processor_architecture_ == "arm")
		return assembly_processor_architecture::arm;
	if (*processor_architecture_ == "arm64")
		return assembly_processor_architecture::arm64;
	if (*processor_architecture_ == "*")
		return assembly_processor_architecture::any;

	return assembly_processor_architecture::unknown;
}

template<typename... Bases>
assembly_version assembly_identity_base<Bases...>
	::get_version() const
{
	return assembly_version{ parse_full_version(version_) };
}

template<typename... Bases>
std::optional<public_key_token_type> assembly_identity_base<Bases...>
	::get_public_key_token() const
{
	if (!public_key_token_)
		return {};

	if (public_key_token_->size() != 16u)
		throw pe_error(manifest_errc::invalid_hex_string);

	public_key_token_type result;
	const char* begin = public_key_token_->data();
	for (std::size_t i = 0; i != 16u; i += 2u, begin += 2u) {
		std::uint8_t value{};
		auto res = std::from_chars(begin, begin + 2u, value, 16u);
		if (res.ptr != begin + 2u || res.ec != std::errc{})
			throw pe_error(manifest_errc::invalid_hex_string);
		result[i / 2u] = std::byte{ value };
	}

	return result;
}

template<typename... Bases>
std::unordered_set<assembly_supported_os> assembly_supported_os_list_base<Bases...>
	::get_list() const
{
	static constexpr guid_version_type guid_vista{
		"{e2011457-1546-43c5-a5fe-008deee3d3f0}",
		assembly_supported_os::win_vista_server2008 };
	static constexpr guid_version_type guid_7{
		"{35138b9a-5d96-4fbd-8e2d-a2440225f93a}",
		assembly_supported_os::win7_server2008_r2 };
	static constexpr guid_version_type guid_8{
		"{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}",
		assembly_supported_os::win8_server2012 };
	static constexpr guid_version_type guid_8_1{
		"{1f676c76-80e1-4239-95bb-83d0f6d0da78}",
		assembly_supported_os::win8_1_server2012_r2 };
	static constexpr guid_version_type guid_10{
		"{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}",
		assembly_supported_os::win10_win11_server2016_server2019_server2022 };

	std::unordered_set<assembly_supported_os> result;
	result.reserve(supported_os_.size());
	bool all_os_found = true;
	for (const auto& guid : supported_os_)
	{
		bool os_found = false;
		for (const auto& [target_guid, version] : {
			guid_vista, guid_7, guid_8, guid_8_1, guid_10 })
		{
			if (utilities::iequal(target_guid, guid))
			{
				os_found = true;
				result.insert(version);
				break;
			}
		}

		if (!os_found)
			all_os_found = false;
	}

	if (!all_os_found)
		result.insert(assembly_supported_os::unsupported);

	return result;
}

template<typename... Bases>
std::optional<full_version> assembly_supported_os_list_base<Bases...>
	::get_max_tested_os_version()
{
	if (!max_tested_os_version_)
		return {};

	return parse_full_version(*max_tested_os_version_);
}

active_code_page_type active_code_page::get_code_page() const noexcept
{
	if (utilities::iequal(name_, "UTF-8"))
		return utf8_code_page_tag{};

	if (utilities::iequal(name_, "Legacy"))
		return legacy_code_page_tag{};

	auto name = name_;
	utilities::to_lower_inplace(name);
	auto info = get_lcid_info(name);
	if (info)
		return *info;

	return {};
}

dpi_aware_value dpi_awareness::get_dpi_aware_value() const noexcept
{
	if (!dpi_aware_)
		return dpi_aware_value::absent;

	if (utilities::iequal(*dpi_aware_, "true"))
		return dpi_aware_value::dpi_aware;

	if (utilities::iequal(*dpi_aware_, "false"))
		return dpi_aware_value::dpi_unaware;

	if (utilities::iequal(*dpi_aware_, "true/pm"))
		return dpi_aware_value::dpi_aware_true_per_monitor;

	if (utilities::iequal(*dpi_aware_, "per monitor"))
		return dpi_aware_value::dpi_aware_per_monitor;

	return dpi_aware_value::absent;
}

dpi_awareness_value dpi_awareness::get_dpi_awareness_value() const noexcept
{
	if (!dpi_awareness_)
		return dpi_awareness_value::absent;

	static constexpr std::string_view delim{ "," };
	for (const auto& word : std::views::split(*dpi_awareness_, delim)) {
		std::string_view elem(word.begin(), word.end());
		utilities::trim(elem);

		if (utilities::iequal(elem, "system"))
			return dpi_awareness_value::dpi_aware_system;
		if (utilities::iequal(elem, "permonitor"))
			return dpi_awareness_value::dpi_aware_per_monitor;
		if (utilities::iequal(elem, "permonitorv2"))
			return dpi_awareness_value::dpi_aware_per_monitor_v2;
		if (utilities::iequal(elem, "unaware"))
			return dpi_awareness_value::dpi_unaware;
	}

	return dpi_awareness_value::unrecognized;
}

heap_type_value heap_type::get_heap_type() const noexcept
{
	if (utilities::iequal(heap_type_, "SegmentHeap"))
		return heap_type_value::segment_heap;

	return heap_type_value::unknown;
}

requested_execution_level requested_privileges::get_level() const noexcept
{
	if (level_ == "asInvoker")
		return requested_execution_level::as_invoker;
	if (level_ == "requireAdministrator")
		return requested_execution_level::require_administrator;
	if (level_ == "highestAvailable")
		return requested_execution_level::highest_available;

	return requested_execution_level::unknown;
}

std::optional<bool> requested_privileges::get_ui_access() const noexcept
{
	if (ui_access_ == "true")
		return true;

	if (ui_access_ == "false")
		return false;

	return {};
}

namespace
{
com_threading_model string_to_threading_model(
	const std::optional<std::string>& threading_model)
{
	if (!threading_model)
		return com_threading_model::unspecified;

	if (utilities::iequal(*threading_model, "Neutral"))
		return com_threading_model::neutral;
	if (utilities::iequal(*threading_model, "Apartment"))
		return com_threading_model::apartment;
	if (utilities::iequal(*threading_model, "Free"))
		return com_threading_model::free;
	if (utilities::iequal(*threading_model, "Both"))
		return com_threading_model::both;

	return com_threading_model::unknown;
}
} //namespace

template<typename... Bases>
com_threading_model com_class_base<Bases... > ::get_threading_model() const noexcept
{
	return string_to_threading_model(threading_model_);
}

template<typename... Bases>
guid com_class_base<Bases...>::get_clsid() const
{
	return parse_guid(clsid_);
}

template<typename... Bases>
std::optional<guid> com_class_base<Bases...>::get_tlbid() const
{
	if (!tlbid_)
		return {};

	return parse_guid(*tlbid_);
}

template<typename... Bases>
std::optional<com_progid> com_class_base<Bases...>::get_progid() const
{
	if (!progid_)
		return {};

	return com_progid::parse(*progid_);
}

namespace
{
bool is_valid_progid_char(char c) noexcept
{
	static_assert('F' == 70 && 'Z' == 90 && 'f' == 102 && 'z' == 122);
	return (c >= 'a' && c <= 'z'
		|| c >= 'A' && c <= 'Z'
		|| c >= '0' && c <= '9');
}

bool has_invalid_progid_characters(std::string_view s) noexcept
{
	return std::find_if_not(s.cbegin(), s.cend(), is_valid_progid_char)
		!= s.cend();
}
} //namespace

namespace
{
ole_misc::value misc_status_string_to_value(const std::optional<std::string>& str)
{
	if (!str)
		return {};

	static const std::unordered_map<std::string_view, ole_misc::value> value_map{
		{ "recomposeonresize", ole_misc::recomposeonresize },
		{ "onlyiconic", ole_misc::onlyiconic },
		{ "insertnotreplace", ole_misc::insertnotreplace },
		{ "static", ole_misc::static_value },
		{ "cantlinkinside", ole_misc::cantlinkinside },
		{ "canlinkbyole1", ole_misc::canlinkbyole1 },
		{ "islinkobject", ole_misc::islinkobject },
		{ "insideout", ole_misc::insideout },
		{ "activatewhenvisible", ole_misc::activatewhenvisible },
		{ "renderingisdeviceindependent", ole_misc::renderingisdeviceindependent },
		{ "invisibleatruntime", ole_misc::invisibleatruntime },
		{ "alwaysrun", ole_misc::alwaysrun },
		{ "actslikebutton", ole_misc::actslikebutton },
		{ "actslikelabel", ole_misc::actslikelabel },
		{ "nouiactivate", ole_misc::nouiactivate },
		{ "alignable", ole_misc::alignable },
		{ "simpleframe", ole_misc::simpleframe },
		{ "setclientsitefirst", ole_misc::setclientsitefirst },
		{ "imemode", ole_misc::imemode },
		{ "ignoreativatewhenvisible", ole_misc::ignoreativatewhenvisible },
		{ "wantstomenumerge", ole_misc::wantstomenumerge },
		{ "supportsmultilevelundo", ole_misc::supportsmultilevelundo }
	};

	std::uint32_t result{};
	static constexpr std::string_view delim{ "," };
	for (const auto& word : std::views::split(*str, delim)) {
		std::string_view elem(word.begin(), word.end());
		utilities::trim(elem);
		std::string elem_copy(elem);
		utilities::to_lower_inplace(elem_copy);
		auto it = value_map.find(elem_copy);
		if (it != value_map.cend())
			result |= it->second;
		else
			result |= ole_misc::unknown;
	}

	return static_cast<ole_misc::value>(result);
}
} //namespace

template<typename... Bases>
ole_misc::value com_class_base<Bases...>::get_misc_status() const
{
	return misc_status_string_to_value(misc_status_);
}

template<typename... Bases>
ole_misc::value com_class_base<Bases...>::get_misc_status_icon() const
{
	return misc_status_string_to_value(misc_status_icon_);
}

template<typename... Bases>
ole_misc::value com_class_base<Bases...>::get_misc_status_content() const
{
	return misc_status_string_to_value(misc_status_content_);
}

template<typename... Bases>
ole_misc::value com_class_base<Bases...>::get_misc_status_docprint() const
{
	return misc_status_string_to_value(misc_status_docprint_);
}

template<typename... Bases>
ole_misc::value com_class_base<Bases...>::get_misc_status_thumbnail() const
{
	return misc_status_string_to_value(misc_status_thumbnail_);
}

bool com_progid::is_valid() const noexcept
{
	if (program_.empty() || component_.empty())
		return false;

	static constexpr std::size_t max_progid_length = 39u;
	std::size_t length = program_.size() + 1u + component_.size();
	if (version_)
	{
		if (version_->empty())
			return false;

		length += 1u + version_->size();
	}

	if (length > max_progid_length)
		return false;

	if (std::isdigit(program_[0]))
		return false;

	if (has_invalid_progid_characters(program_)
		|| (version_ && has_invalid_progid_characters(*version_))
		|| has_invalid_progid_characters(component_))
	{
		return false;
	}

	return true;
}

com_progid com_progid::parse(std::string_view str)
{
	static constexpr std::string_view delim{ "." };
	auto view = std::views::split(str, delim);
	auto it = view.begin();
	auto end = view.end();
	com_progid result;
	if (it != end)
	{
		result.get_program().assign((*it).begin(), (*it).end());
		if (++it != end) {
			result.get_component().assign((*it).begin(), (*it).end());
			if (++it != end) {
				result.get_version().emplace(it.base(), str.end());
			}

			if (!result.is_valid())
				throw pe_error(manifest_errc::invalid_com_progid);

			return result;
		}
	}

	throw pe_error(manifest_errc::invalid_com_progid);
}

template<typename... Bases>
guid com_typelib_base<Bases...>::get_tlbid() const
{
	return parse_guid(tlbid_);
}

template<typename... Bases>
short_version com_typelib_base<Bases...>::get_version() const
{
	return parse_short_version(version_);
}

template<typename... Bases>
std::optional<lcid_type> com_typelib_base<Bases...>::get_resource_id() const
{
	if (!resource_id_)
		return {};

	std::uint16_t value{};
	auto end = resource_id_->data() + resource_id_->size();
	auto result = std::from_chars(resource_id_->data(), end, value, 16u);
	if (result.ec != std::errc{} || result.ptr != end)
		throw pe_error(manifest_errc::invalid_lcid_string);

	return value;
}

template<typename... Bases>
std::optional<com_typelib_flags::value> com_typelib_base<Bases...>::get_flags() const
{
	if (!flags_)
		return {};

	std::uint32_t result{};
	static constexpr std::string_view delim{ "," };
	for (const auto& word : std::views::split(*flags_, delim)) {
		std::string_view elem(word.begin(), word.end());
		utilities::trim(elem);
		if (utilities::iequal(elem, "RESTRICTED"))
			result |= com_typelib_flags::restricted;
		else if (utilities::iequal(elem, "CONTROL"))
			result |= com_typelib_flags::control;
		else if (utilities::iequal(elem, "HIDDEN"))
			result |= com_typelib_flags::hidden;
		else if (utilities::iequal(elem, "HASDISKIMAGE"))
			result |= com_typelib_flags::has_disk_image;
		else
			result |= com_typelib_flags::unknown;
	}

	return static_cast<com_typelib_flags::value>(result);
}

template<typename... Bases>
guid com_interface_external_proxy_stub_base<Bases...>::get_iid() const
{
	return parse_guid(iid_);
}

template<typename... Bases>
std::optional<guid> com_interface_external_proxy_stub_base<Bases...>
	::get_base_interface() const
{
	if (!base_interface_)
		return {};

	return parse_guid(*base_interface_);
}

template<typename... Bases>
std::optional<guid> com_interface_external_proxy_stub_base<Bases...>
	::get_tlbid() const
{
	if (!tlbid_)
		return {};

	return parse_guid(*tlbid_);
}

template<typename... Bases>
std::optional<guid> com_interface_external_proxy_stub_base<Bases...>
	::get_proxy_stub_clsid32() const
{
	if (!proxy_stub_clsid32_)
		return {};

	return parse_guid(*proxy_stub_clsid32_);
}

template<typename... Bases>
std::optional<std::uint64_t> com_interface_external_proxy_stub_base<Bases...>
	::get_num_methods() const
{
	if (!num_methods_)
		return {};

	std::uint64_t value{};
	auto end = num_methods_->data() + num_methods_->size();
	auto res = std::from_chars(num_methods_->data(), end, value);
	if (res.ptr != end || res.ec != std::errc{})
		throw pe_error(manifest_errc::invalid_num_methods_string);

	return value;
}

template<typename... Bases>
com_threading_model com_interface_proxy_stub_base<Bases...>
	::get_threading_model() const noexcept
{
	return string_to_threading_model(threading_model_);
}

template<typename... Bases>
bool window_class_base<Bases...>::is_versioned() const
{
	if (!versioned_)
		return true;

	if (utilities::iequal(*versioned_, "yes"))
		return true;
	else if (utilities::iequal(*versioned_, "no"))
		return false;
	else
		throw pe_error(manifest_errc::invalid_bool_string);
}

template<typename... Bases>
std::optional<std::vector<std::byte>>
	assembly_file_base<Bases...>::get_hash() const
{
	if (!hash_)
		return {};

	if (hash_->size() % 2)
		throw pe_error(manifest_errc::invalid_hex_string);

	std::vector<std::byte> result;
	result.reserve(hash_->size() / 2u);
	const char* begin = hash_->data();
	for (std::size_t i = 0; i != hash_->size(); i += 2u, begin += 2u) {
		std::uint8_t value{};
		auto res = std::from_chars(begin, begin + 2u, value, 16u);
		if (res.ptr != begin + 2u || res.ec != std::errc{})
			throw pe_error(manifest_errc::invalid_hex_string);
		result.emplace_back(std::byte{ value });
	}
	return std::move(result);
}

template<typename... Bases>
assembly_file_hash_algorithm assembly_file_base<Bases...>
	::get_hash_algorithm() const noexcept
{
	if (!hash_algorithm_)
		return assembly_file_hash_algorithm::unspecified;

	if (utilities::iequal(*hash_algorithm_, "sha1"))
		return assembly_file_hash_algorithm::sha1;
	if (utilities::iequal(*hash_algorithm_, "sha256"))
		return assembly_file_hash_algorithm::sha256;

	return assembly_file_hash_algorithm::unknown;
}

template<typename... Bases>
std::optional<std::uint64_t> assembly_file_base<Bases...>
	::get_size() const
{
	if (!size_)
		return {};

	auto end = size_->data() + size_->size();
	std::uint64_t size{};
	auto res = std::from_chars(size_->data(), end, size);
	if (res.ec != std::errc{} || res.ptr != end)
		throw pe_error(manifest_errc::invalid_file_size_string);

	return size;
}

namespace
{
constexpr std::string_view asmv1_ns("urn:schemas-microsoft-com:asm.v1");
constexpr std::string_view asmv2_ns("urn:schemas-microsoft-com:asm.v2");
constexpr std::string_view asmv3_ns("urn:schemas-microsoft-com:asm.v3");
constexpr std::string_view windows_settings2005_ns("http://schemas.microsoft.com/SMI/2005/WindowsSettings");
constexpr std::string_view windows_settings2011_ns("http://schemas.microsoft.com/SMI/2011/WindowsSettings");
constexpr std::string_view windows_settings2013_ns("http://schemas.microsoft.com/SMI/2013/WindowsSettings");
constexpr std::string_view windows_settings2016_ns("http://schemas.microsoft.com/SMI/2016/WindowsSettings");
constexpr std::string_view windows_settings2017_ns("http://schemas.microsoft.com/SMI/2017/WindowsSettings");
constexpr std::string_view windows_settings2019_ns("http://schemas.microsoft.com/SMI/2019/WindowsSettings");
constexpr std::string_view windows_settings2020_ns("http://schemas.microsoft.com/SMI/2020/WindowsSettings");
constexpr std::string_view compatibilityv1_ns("urn:schemas-microsoft-com:compatibility.v1");
constexpr std::string_view msixv1_ns("urn:schemas-microsoft-com:msix.v1");

template<typename Getter, manifest_errc Errc
	= static_cast<manifest_errc>(0u), typename Result>
void read_attribute(Result& result, const manifest_node_interface& elem,
	std::string_view ns, std::string_view name)
{
	const auto* attr = elem.get_attribute(ns, name);
	if (attr)
	{
		Getter{}(result) = attr->get_value();
	}
	else
	{
		if constexpr (static_cast<std::uint32_t>(Errc) != 0u)
			result.add_error(Errc);
	}
}

bool load_no_inherit(native_manifest_details& result,
	manifest_node_iterator_interface& assembly_elem_it)
{
	const auto* no_inherit = assembly_elem_it.first_child(asmv1_ns, "noInherit");
	bool multiple_no_inherit = no_inherit && assembly_elem_it.next_child();
	const auto* no_inheritable = assembly_elem_it.first_child(asmv1_ns, "noInheritable");
	bool multiple_no_inheritable = no_inheritable && assembly_elem_it.next_child();
	if ((no_inherit && no_inheritable) || multiple_no_inherit || multiple_no_inheritable)
	{
		result.add_error(manifest_errc::multiple_no_inherit_elements);
		return false;
	}

	if (!no_inherit && !no_inheritable)
		return false;

	if (no_inherit)
		result.set_no_inherit(assembly_no_inherit::no_inherit);
	else
		result.set_no_inherit(assembly_no_inherit::no_inheritable);

	if ((no_inherit ? no_inherit : no_inheritable)->get_node_index() != 0u)
	{
		result.add_error(manifest_errc::no_inherit_element_not_first);
		return false;
	}

	return true;
}

void load_assembly_identity(assembly_identity_details& identity,
	const manifest_node_interface& assembly_identity_elem)
{
	read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_type_raw(); }),
		manifest_errc::absent_assembly_identity_type>
		(identity, assembly_identity_elem, asmv1_ns, "type");
	read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_name(); }),
		manifest_errc::absent_assembly_identity_name>
		(identity, assembly_identity_elem, asmv1_ns, "name");
	read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_language_raw(); })>
		(identity, assembly_identity_elem, asmv1_ns, "language");
	read_attribute<decltype([](auto& r) -> decltype(auto)
		{ return r.get_processor_architecture_raw(); })>
		(identity, assembly_identity_elem, asmv1_ns, "processorArchitecture");
	read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_version_raw(); }),
		manifest_errc::absent_assembly_identity_version>
		(identity, assembly_identity_elem, asmv1_ns, "version");
	read_attribute<decltype([](auto& r) -> decltype(auto)
		{ return r.get_public_key_token_raw(); })>
		(identity, assembly_identity_elem, asmv1_ns, "publicKeyToken");
}

void load_compatibility(native_manifest_details& result,
	manifest_node_iterator_interface& assembly_elem_it)
{
	const auto* compatibility_elem = assembly_elem_it.first_child(
		compatibilityv1_ns, "compatibility");
	if (!compatibility_elem)
		return;

	if (assembly_elem_it.next_child())
		result.add_error(manifest_errc::multiple_compatibility_elements);

	auto compatibility_elem_it = compatibility_elem->get_iterator();
	const auto* application_elem = compatibility_elem_it->first_child(
		compatibilityv1_ns, "application");
	if (!application_elem)
		return;

	auto application_elem_it = application_elem->get_iterator();
	const auto* supported_os_elem = application_elem_it->first_child(
		compatibilityv1_ns, "supportedOS");
	auto& compatibility = result.get_supported_os_list().emplace();
	while (supported_os_elem)
	{
		const auto* id = supported_os_elem->get_attribute(compatibilityv1_ns, "Id");
		if (id)
			compatibility.get_list_raw().emplace_back(id->get_value());

		supported_os_elem = application_elem_it->next_child();
	}

	const auto* max_tested_elem = application_elem_it->first_child(
		compatibilityv1_ns, "maxversiontested");
	if (max_tested_elem)
	{
		if (application_elem_it->next_child())
			compatibility.add_error(manifest_errc::multiple_maxversiontested_elements);

		const auto* id = max_tested_elem->get_attribute(compatibilityv1_ns, "Id");
		if (id)
			compatibility.get_max_tested_os_version_raw() = id->get_value();
	}
}

void load_dependencies(native_manifest_details& result,
	manifest_node_iterator_interface& assembly_elem_it)
{
	const auto* dependency_elem = assembly_elem_it.first_child(
		asmv1_ns, "dependency");
	auto& dependencies = result.get_dependencies();
	while (dependency_elem)
	{
		auto dependency_elem_it = dependency_elem->get_iterator();
		const auto* dependent_assembly_elem = dependency_elem_it->first_child(
			asmv1_ns, "dependentAssembly");

		if (!dependent_assembly_elem)
		{
			dependency_elem = assembly_elem_it.next_child();
			result.add_error(manifest_errc::invalid_dependencies);
			continue;
		}

		auto dependent_assembly_elem_it = dependent_assembly_elem->get_iterator();
		const auto* assembly_identity_elem = dependent_assembly_elem_it->first_child(
			asmv1_ns, "assemblyIdentity");
		if (!assembly_identity_elem)
		{
			dependency_elem = assembly_elem_it.next_child();
			result.add_error(manifest_errc::invalid_dependencies);
			continue;
		}

		auto& dependency = dependencies.emplace_back();
		if (dependent_assembly_elem->get_node_index() != 0u
			|| assembly_identity_elem->get_node_index() != 0u)
		{
			dependency.add_error(
				manifest_errc::invalid_dependend_assembly_element_position);
		}
		if (dependency_elem_it->next_child()
			|| dependent_assembly_elem_it->next_child())
		{
			dependency.add_error(manifest_errc::multiple_assembly_identities);
		}

		load_assembly_identity(dependency, *assembly_identity_elem);
		dependency_elem = assembly_elem_it.next_child();
	}
}

void load_com_classes(assembly_file_details& result,
	manifest_node_iterator_interface& file_elem_it)
{
	auto& com_classes = result.get_com_classes();
	const auto* com_class_elem = file_elem_it.first_child(asmv1_ns, "comClass");
	while (com_class_elem)
	{
		auto& com_class = com_classes.emplace_back();
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_description(); })>
			(com_class, *com_class_elem, asmv1_ns, "description");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_clsid_raw(); }),
			manifest_errc::absent_clsid>
			(com_class, *com_class_elem, asmv1_ns, "clsid");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_threading_model_raw(); })>
			(com_class, *com_class_elem, asmv1_ns, "threadingModel");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_tlbid_raw(); })>
			(com_class, *com_class_elem, asmv1_ns, "tlbid");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_progid_raw(); })>
			(com_class, *com_class_elem, asmv1_ns, "progid");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_misc_status_raw(); })>
			(com_class, *com_class_elem, asmv1_ns, "miscStatus");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_misc_status_icon_raw(); }) >
			(com_class, *com_class_elem, asmv1_ns, "miscStatusIcon");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_misc_status_content_raw(); }) >
			(com_class, *com_class_elem, asmv1_ns, "miscStatusContent");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_misc_status_thumbnail_raw(); }) >
			(com_class, *com_class_elem, asmv1_ns, "miscStatusThumbnail");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_misc_status_docprint_raw(); }) >
			(com_class, *com_class_elem, asmv1_ns, "miscStatusDocprint");

		auto com_class_it = com_class_elem->get_iterator();
		const auto* progid_elem = com_class_it->first_child(asmv1_ns, "progid");
		auto& progids = com_class.get_additional_progids();
		while (progid_elem)
		{
			if (progids.emplace_back(utilities::trim_copy(
				progid_elem->get_value())).empty())
			{
				com_class.add_error(manifest_errc::empty_progids);
			}

			progid_elem = com_class_it->next_child();
		}

		com_class_elem = file_elem_it.next_child();
	}
}

void load_typelibs(assembly_file_details& result,
	manifest_node_iterator_interface& file_elem_it)
{
	auto& com_typelibs = result.get_com_typelibs();
	const auto* com_typelib_elem = file_elem_it.first_child(asmv1_ns, "typelib");
	while (com_typelib_elem)
	{
		auto& com_typelib = com_typelibs.emplace_back();
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_tlbid_raw(); }),
			manifest_errc::absent_tlbid>
			(com_typelib, *com_typelib_elem, asmv1_ns, "tlbid");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_version_raw(); }),
			manifest_errc::absent_version>
			(com_typelib, *com_typelib_elem, asmv1_ns, "version");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_help_dir(); }),
			manifest_errc::absent_helpdir>
			(com_typelib, *com_typelib_elem, asmv1_ns, "helpdir");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_resource_id_raw(); })>
			(com_typelib, *com_typelib_elem, asmv1_ns, "resourceid");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_flags_raw(); }) >
			(com_typelib, *com_typelib_elem, asmv1_ns, "flags");

		com_typelib_elem = file_elem_it.next_child();
	}
}

void load_window_classes(assembly_file_details& result,
	manifest_node_iterator_interface& file_elem_it)
{
	auto& window_classes = result.get_window_classes();
	const auto* window_class_elem = file_elem_it.first_child(asmv1_ns, "windowClass");
	while (window_class_elem)
	{
		auto& window_class = window_classes.emplace_back();
		window_class.get_class() = utilities::trim_copy(window_class_elem->get_value());
		if (window_class.get_class().empty())
			window_class.add_error(manifest_errc::empty_window_class);
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.is_versioned_raw(); })>
			(window_class, *window_class_elem, asmv1_ns, "versioned");

		window_class_elem = file_elem_it.next_child();
	}
}

template<typename ProxyStubs>
void load_com_interface_proxy_stubs(ProxyStubs& stubs,
	manifest_node_iterator_interface& elem_it, std::string_view elemName)
{
	auto* stub_elem = elem_it.first_child(asmv1_ns, elemName);
	while (stub_elem)
	{
		auto& stub = stubs.emplace_back();
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_iid_raw(); }),
			manifest_errc::absent_iid>
			(stub, *stub_elem, asmv1_ns, "iid");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_name(); })>
			(stub, *stub_elem, asmv1_ns, "name");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_tlbid_raw(); })>
			(stub, *stub_elem, asmv1_ns, "tlbid");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_base_interface_raw(); })>
			(stub, *stub_elem, asmv1_ns, "baseInterface");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_num_methods_raw(); })>
			(stub, *stub_elem, asmv1_ns, "numMethods");
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_proxy_stub_clsid32_raw(); })>
			(stub, *stub_elem, asmv1_ns, "proxyStubClsid32");
		if constexpr (std::is_same_v<decltype(stub), com_interface_proxy_stub_details&>)
		{
			read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_threading_model_raw(); })>
				(stub, *stub_elem, asmv1_ns, "threadingModel");
		}

		stub_elem = elem_it.next_child();
	}
}

void load_files(native_manifest_details& result,
	manifest_node_iterator_interface& assembly_elem_it)
{
	const auto* file_elem = assembly_elem_it.first_child(
		asmv1_ns, "file");
	auto& files = result.get_files();
	while (file_elem)
	{
		auto& file = files.emplace_back();
		
		read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_name(); }),
			manifest_errc::absent_file_name>
			(file, *file_elem, asmv1_ns, "name");
		read_attribute<decltype([](auto& r) -> decltype(auto) {
			return r.get_hash_algorithm_raw(); })>
			(file, *file_elem, asmv1_ns, "hashalg");
		read_attribute<decltype([](auto& r) -> decltype(auto) {
			return r.get_hash_raw(); })>
			(file, *file_elem, asmv1_ns, "hash");
		read_attribute<decltype([](auto& r) -> decltype(auto) {
			return r.get_size_raw(); })>
			(file, *file_elem, asmv2_ns, "size");

		auto file_elem_it = file_elem->get_iterator();
		load_com_classes(file, *file_elem_it);
		load_typelibs(file, *file_elem_it);
		load_window_classes(file, *file_elem_it);
		load_com_interface_proxy_stubs(file.get_com_interface_proxy_stubs(),
			*file_elem_it, "comInterfaceProxyStub");

		file_elem = assembly_elem_it.next_child();
	}
}

template<auto Setter, manifest_errc MultipleElemErrc, manifest_errc InvalidValueErrc>
void load_bool_option(native_manifest_details& result,
	manifest_node_iterator_interface& elem_it,
	std::string_view ns, std::string_view name)
{
	const auto* bool_elem = elem_it.first_child(ns, name);
	if (!bool_elem)
		return;
	if (elem_it.next_child())
	{
		result.add_error(MultipleElemErrc);
		return;
	}

	auto value = utilities::trim_copy(bool_elem->get_value());
	if (utilities::iequal(value, "true"))
	{
		(result.*Setter)(true);
		return;
	}

	if (!utilities::iequal(value, "false"))
		result.add_error(InvalidValueErrc);
}

template<typename SetterLambda, manifest_errc MultipleElemErrc,
	manifest_errc AbsentElemErrc = static_cast<manifest_errc>(0u)>
void load_option(native_manifest_details& result,
	manifest_node_iterator_interface& elem_it,
	std::string_view ns, std::string_view name)
{
	const auto* elem = elem_it.first_child(ns, name);
	if (!elem)
	{
		if constexpr (AbsentElemErrc != static_cast<manifest_errc>(0))
			result.add_error(AbsentElemErrc);

		return;
	}

	if (elem_it.next_child())
	{
		result.add_error(MultipleElemErrc);
		return;
	}

	SetterLambda{}(result) = utilities::trim_copy(elem->get_value());
}

void load_windows_settings(native_manifest_details& result,
	manifest_node_iterator_interface& assembly_elem_it)
{
	const auto* application_elem = assembly_elem_it.first_child(
		asmv3_ns, "application");
	if (!application_elem)
		return;

	if (assembly_elem_it.next_child())
		result.add_error(manifest_errc::multiple_application_elements);

	auto application_elem_it = application_elem->get_iterator();
	const auto* windows_settings_elem = application_elem_it->first_child(
		asmv3_ns, "windowsSettings");
	if (!windows_settings_elem)
		return;

	if (application_elem_it->next_child())
		result.add_error(manifest_errc::multiple_windows_settings_elements);

	auto windows_settings_elem_it = windows_settings_elem->get_iterator();

	load_option<decltype([](auto& r) -> decltype(auto) {
		return r.get_active_code_page().emplace().get_name(); }),
		manifest_errc::multiple_active_code_page_elements>
			(result, *windows_settings_elem_it, windows_settings2019_ns, "activeCodePage");
	load_bool_option<&native_manifest_details::set_auto_elevate,
		manifest_errc::multiple_auto_elevate_elem,
		manifest_errc::invalid_auto_elevate_elem>(
			result, *windows_settings_elem_it, windows_settings2005_ns, "autoElevate");
	load_bool_option<&native_manifest_details::set_disable_theming,
		manifest_errc::multiple_disable_theming_elem,
		manifest_errc::invalid_disable_theming_elem>(
			result, *windows_settings_elem_it, windows_settings2005_ns, "disableTheming");
	load_bool_option<&native_manifest_details::set_disable_window_filtering,
		manifest_errc::multiple_disable_window_filtering_elem,
		manifest_errc::invalid_disable_window_filtering_elem>(
			result, *windows_settings_elem_it, windows_settings2011_ns, "disableWindowFiltering");
	load_bool_option<&native_manifest_details::set_gdi_scaling,
		manifest_errc::multiple_gdi_scaling_elem,
		manifest_errc::invalid_gdi_scaling_elem>(
			result, *windows_settings_elem_it, windows_settings2017_ns, "gdiScaling");
	load_bool_option<&native_manifest_details::set_high_resolution_scrolling_aware,
		manifest_errc::multiple_high_resolution_scrolling_aware_elem,
		manifest_errc::invalid_high_resolution_scrolling_aware_elem>(
			result, *windows_settings_elem_it, windows_settings2013_ns, "highResolutionScrollingAware");
	load_bool_option<&native_manifest_details::set_ultra_high_resolution_scrolling_aware,
		manifest_errc::multiple_ultra_high_resolution_scrolling_aware_elem,
		manifest_errc::invalid_ultra_high_resolution_scrolling_aware_elem>(
			result, *windows_settings_elem_it, windows_settings2013_ns, "ultraHighResolutionScrollingAware");
	load_bool_option<&native_manifest_details::set_long_path_aware,
		manifest_errc::multiple_long_path_aware_elem,
		manifest_errc::invalid_long_path_aware_elem>(
			result, *windows_settings_elem_it, windows_settings2016_ns, "longPathAware");
	load_bool_option<&native_manifest_details::set_printer_driver_isolation,
		manifest_errc::multiple_printer_driver_isolation_elem,
		manifest_errc::invalid_printer_driver_isolation_elem>(
			result, *windows_settings_elem_it, windows_settings2011_ns, "printerDriverIsolation");
	load_option<decltype([](auto& r) -> decltype(auto) {
		return r.get_dpi_awareness().get_dpi_aware_raw(); }),
		manifest_errc::multiple_dpi_aware_elements>
			(result, *windows_settings_elem_it, windows_settings2005_ns, "dpiAware");
	load_option<decltype([](auto& r) -> decltype(auto) {
		return r.get_dpi_awareness().get_dpi_awareness_raw(); }),
		manifest_errc::multiple_dpi_awareness_elements>
			(result, *windows_settings_elem_it, windows_settings2016_ns, "dpiAwareness");
	load_option<decltype([](auto& r) -> decltype(auto) {
		return r.get_heap_type().emplace().get_heap_type_raw(); }),
		manifest_errc::multiple_heap_type_elements>
			(result, *windows_settings_elem_it, windows_settings2020_ns, "heapType");
}

void load_description(native_manifest_details& result,
	manifest_node_iterator_interface& assembly_elem_it)
{
	const auto* description_elem = assembly_elem_it.first_child(asmv1_ns, "description");
	if (!description_elem)
		return;

	auto& description = result.get_description().emplace();
	while (description_elem)
	{
		auto text = utilities::trim_copy(description_elem->get_value());
		if (!description.empty() && !text.empty())
			description += '\n';
		description.append(text);
		description_elem = assembly_elem_it.next_child();
	}
}

const manifest_node_interface* get_child_elem(native_manifest_details& result,
	manifest_node_iterator_interface& elem_it, std::string_view ns1, std::string_view ns2,
	std::string_view name, manifest_errc multiple_elem_errc)
{
	const auto* child_elem1 = elem_it.first_child(ns1, name);
	const auto* child_elem1_next = child_elem1 ? elem_it.next_child() : nullptr;
	const auto* child_elem2 = elem_it.first_child(ns2, name);
	const auto* child_elem2_next = child_elem2 ? elem_it.next_child() : nullptr;
	if ((child_elem1 && child_elem2)
		|| child_elem1_next || child_elem2_next)
	{
		result.add_error(multiple_elem_errc);
		return nullptr;
	}

	return child_elem1 ? child_elem1 : child_elem2;
}

void load_trust_info(native_manifest_details& result,
	manifest_node_iterator_interface& assembly_elem_it)
{
	const auto* trust_info_elem = get_child_elem(result,
		assembly_elem_it, asmv2_ns, asmv3_ns,
		"trustInfo", manifest_errc::multiple_trust_info_elements);
	if (!trust_info_elem)
		return;

	const auto* security_elem = get_child_elem(result,
		*trust_info_elem->get_iterator(), asmv2_ns, asmv3_ns,
		"security", manifest_errc::multiple_security_elements);
	if (!security_elem)
		return;

	const auto* requested_privileges_elem = get_child_elem(result,
		*security_elem->get_iterator(), asmv2_ns, asmv3_ns,
		"requestedPrivileges", manifest_errc::multiple_requested_privileges_elements);
	if (!requested_privileges_elem)
		return;

	const auto* requested_execution_level_elem = get_child_elem(result,
		*requested_privileges_elem->get_iterator(), asmv2_ns, asmv3_ns,
		"requestedExecutionLevel", manifest_errc::multiple_requested_execution_level_elements);
	if (!requested_execution_level_elem)
		return;

	auto& requested_privileges = result.get_requested_privileges().emplace();
	const auto* level = requested_execution_level_elem->get_attribute(
		requested_execution_level_elem->get_namespace(), "level");
	if (level)
		requested_privileges.get_level_raw() = level->get_value();
	else
		result.add_error(manifest_errc::absent_requested_execution_level);

	const auto* ui_access = requested_execution_level_elem->get_attribute(
		requested_execution_level_elem->get_namespace(), "uiAccess");
	if (ui_access)
		requested_privileges.get_ui_access_raw() = ui_access->get_value();
	else
		result.add_error(manifest_errc::absent_requested_execution_ui_access);
}

void load_msix_identity(native_manifest_details& result,
	manifest_node_iterator_interface& assembly_elem_it)
{
	const auto* msix_identity_elem = assembly_elem_it.first_child(msixv1_ns, "msix");
	if (!msix_identity_elem)
		return;

	if (assembly_elem_it.next_child())
	{
		result.add_error(manifest_errc::multiple_msix_elements);
		return;
	}

	auto& msix_identity = result.get_msix_identity().emplace();
	read_attribute<decltype([](auto& r) -> decltype(auto) {
		return r.get_publisher(); }),
		manifest_errc::absent_msix_publisher>
		(msix_identity, *msix_identity_elem, msixv1_ns, "publisher");
	read_attribute<decltype([](auto& r) -> decltype(auto) {
		return r.get_package_name(); }),
		manifest_errc::absent_msix_package_name>
		(msix_identity, *msix_identity_elem, msixv1_ns, "packageName");
	read_attribute<decltype([](auto& r) -> decltype(auto) {
		return r.get_application_id(); }),
		manifest_errc::absent_msix_application_id>
		(msix_identity, *msix_identity_elem, msixv1_ns, "applicationId");
}
} //namespace

native_manifest_details parse_manifest(const manifest_accessor_interface& accessor)
{
	native_manifest_details result;

	const auto* root = accessor.get_root();
	if (!root)
	{
		result.add_error(manifest_errc::empty_manifest);
		return result;
	}

	const auto* assembly_elem = root->get_iterator()->first_child(asmv1_ns, "assembly");
	if (!assembly_elem)
	{
		result.add_error(manifest_errc::empty_manifest);
		return result;
	}

	read_attribute<decltype([](auto& r) -> decltype(auto) { return r.get_manifest_version(); }),
		manifest_errc::absent_manifest_version>
		(result, *assembly_elem, asmv1_ns, "manifestVersion");
	if (result.get_manifest_version() != native_manifest::supported_manifest_version)
		result.add_error(manifest_errc::invalid_manifest_version);

	auto assembly_elem_it = assembly_elem->get_iterator();
	bool has_no_inherit = load_no_inherit(result, *assembly_elem_it);

	const auto* assembly_identity_elem = assembly_elem_it->first_child(asmv1_ns, "assemblyIdentity");
	if (!assembly_identity_elem)
	{
		result.add_error(manifest_errc::absent_assembly_identity);
	}
	else
	{
		auto& identity = result.get_assembly_identity();
		if (assembly_identity_elem->get_node_index() != static_cast<std::size_t>(has_no_inherit))
			identity.add_error(manifest_errc::invalid_assembly_identity_element_position);
		if (assembly_elem_it->next_child())
			identity.add_error(manifest_errc::multiple_assembly_identities);

		load_assembly_identity(identity, *assembly_identity_elem);
	}

	load_compatibility(result, *assembly_elem_it);
	load_dependencies(result, *assembly_elem_it);
	load_files(result, *assembly_elem_it);
	load_windows_settings(result, *assembly_elem_it);
	load_description(result, *assembly_elem_it);
	load_com_interface_proxy_stubs(
		result.get_com_interface_external_proxy_stubs(),
		*assembly_elem_it, "comInterfaceExternalProxyStub");
	load_msix_identity(result, *assembly_elem_it);
	load_trust_info(result, *assembly_elem_it);
	return result;
}

template assembly_identity_base<>;
template assembly_identity_base<error_list>;
template assembly_supported_os_list_base<>;
template assembly_supported_os_list_base<error_list>;
template assembly_file_base<>;
template assembly_file_base<error_list>;
template com_class_base<>;
template com_class_base<error_list>;
template com_typelib_base<>;
template com_typelib_base<error_list>;
template com_interface_external_proxy_stub_base<>;
template com_interface_external_proxy_stub_base<error_list>;
template com_interface_proxy_stub_base<>;
template com_interface_proxy_stub_base<error_list>;
template window_class_base<>;
template window_class_base<error_list>;

} //namespace pe_bliss::resources
