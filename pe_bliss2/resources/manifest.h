#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <variant>
#include <vector>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/resources/guid.h"
#include "pe_bliss2/resources/lcid.h"
#include "pe_bliss2/resources/version.h"

namespace pe_bliss::resources
{

enum class manifest_errc
{
	invalid_manifest_version = 1,
	invalid_version_string,
	invalid_hex_string,
	invalid_com_progid,
	invalid_lcid_string,
	invalid_bool_string,
	empty_manifest,
	absent_manifest_version,
	no_inherit_element_not_first,
	absent_assembly_identity,
	invalid_assembly_identity_element_position,
	multiple_assembly_identities,
	multiple_no_inherit_elements,
	absent_assembly_identity_type,
	absent_assembly_identity_name,
	absent_assembly_identity_version,
	multiple_compatibility_elements,
	multiple_maxversiontested_elements,
	invalid_dependencies,
	invalid_dependend_assembly_element_position,
	invalid_file_size_string,
	absent_file_name,
	absent_clsid,
	absent_tlbid,
	absent_version,
	absent_helpdir,
	absent_iid,
	invalid_num_methods_string,
	multiple_application_elements,
	multiple_windows_settings_elements,
	multiple_active_code_page_elements,
	multiple_auto_elevate_elem,
	invalid_auto_elevate_elem,
	multiple_disable_theming_elem,
	invalid_disable_theming_elem,
	multiple_disable_window_filtering_elem,
	invalid_disable_window_filtering_elem,
	multiple_gdi_scaling_elem,
	invalid_gdi_scaling_elem,
	multiple_high_resolution_scrolling_aware_elem,
	invalid_high_resolution_scrolling_aware_elem,
	multiple_ultra_high_resolution_scrolling_aware_elem,
	invalid_ultra_high_resolution_scrolling_aware_elem,
	multiple_long_path_aware_elem,
	invalid_long_path_aware_elem,
	multiple_printer_driver_isolation_elem,
	invalid_printer_driver_isolation_elem,
	multiple_dpi_aware_elements,
	multiple_dpi_awareness_elements,
	multiple_heap_type_elements,
	multiple_trust_info_elements,
	multiple_security_elements,
	multiple_requested_privileges_elements,
	multiple_requested_execution_level_elements,
	absent_requested_execution_level,
	absent_requested_execution_ui_access,
	absent_msix_publisher,
	absent_msix_package_name,
	absent_msix_application_id,
	multiple_msix_elements,
	empty_progids,
	empty_window_class
};

std::error_code make_error_code(manifest_errc) noexcept;

enum class assembly_identity_type
{
	win32,
	unknown
};

enum class assembly_processor_architecture
{
	x86,
	amd64,
	arm,
	arm64,
	any,
	unspecified,
	unknown
};

struct [[nodiscard]] assembly_version : public full_version
{
	[[nodiscard]]
	constexpr bool is_compatible_with(
		const assembly_version& other) const noexcept
	{
		return major == other.major && minor == other.minor;
	}
};

using public_key_token_type = std::array<std::byte, 8u>;

template<typename... Bases>
class [[nodiscard]] assembly_identity_base : public Bases...
{
public:
	[[nodiscard]]
	const std::string& get_type_raw() const& noexcept;
	[[nodiscard]]
	std::string& get_type_raw() & noexcept;
	[[nodiscard]]
	std::string get_type_raw() && noexcept;
	[[nodiscard]]
	assembly_identity_type get_type() const noexcept;

	[[nodiscard]]
	const std::string& get_name() const& noexcept;
	[[nodiscard]]
	std::string& get_name() & noexcept;
	[[nodiscard]]
	std::string get_name() && noexcept;

	[[nodiscard]]
	const std::optional<std::string>& get_language_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_language_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_language_raw() && noexcept;
	[[nodiscard]]
	std::optional<lcid_info> get_language() const noexcept;

	[[nodiscard]]
	const std::optional<std::string>& get_processor_architecture_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_processor_architecture_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_processor_architecture_raw() && noexcept;
	[[nodiscard]]
	assembly_processor_architecture get_processor_architecture() const noexcept;

	[[nodiscard]]
	const std::string& get_version_raw() const& noexcept;
	[[nodiscard]]
	std::string& get_version_raw() & noexcept;
	[[nodiscard]]
	std::string get_version_raw() && noexcept;
	[[nodiscard]]
	assembly_version get_version() const;

	[[nodiscard]]
	const std::optional<std::string>& get_public_key_token_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_public_key_token_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_public_key_token_raw() && noexcept;
	[[nodiscard]]
	std::optional<public_key_token_type> get_public_key_token() const;

private:
	std::string type_;
	std::string name_;
	std::optional<std::string> language_;
	std::optional<std::string> processor_architecture_;
	std::string version_;
	std::optional<std::string> public_key_token_;
};

enum class assembly_supported_os
{
	win_vista_server2008,
	win7_server2008_r2,
	win8_server2012,
	win8_1_server2012_r2,
	win10_win11_server2016_server2019_server2022,
	unsupported
};

template<typename... Bases>
class [[nodiscard]] assembly_supported_os_list_base : public Bases...
{
public:
	[[nodiscard]]
	const std::vector<std::string>& get_list_raw() const& noexcept;
	[[nodiscard]]
	std::vector<std::string>& get_list_raw() & noexcept;
	[[nodiscard]]
	std::vector<std::string> get_list_raw() && noexcept;
	[[nodiscard]]
	std::unordered_set<assembly_supported_os> get_list() const;

	[[nodiscard]]
	const std::optional<std::string>& get_max_tested_os_version_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_max_tested_os_version_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_max_tested_os_version_raw() && noexcept;
	[[nodiscard]]
	std::optional<full_version> get_max_tested_os_version();

private:
	std::vector<std::string> supported_os_;
	std::optional<std::string> max_tested_os_version_;
};

// https://learn.microsoft.com/en-us/windows/win32/cossdk/threading-model-attribute
enum class com_threading_model
{
	unspecified,
	apartment,
	both,
	free,
	neutral,
	unknown
};

// https://learn.microsoft.com/en-us/windows/win32/com/-progid--key
class [[nodiscard]] com_progid
{
public:
	com_progid() = default;

	template<typename T1, typename T2, typename T3>
	com_progid(T1&& program, T2&& component, T3&& version);
	template<typename T1, typename T2>
	com_progid(T1&& program, T2&& component);

	[[nodiscard]]
	inline const std::string& get_program() const& noexcept;
	[[nodiscard]]
	inline std::string& get_program() & noexcept;
	[[nodiscard]]
	inline std::string get_program() && noexcept;

	[[nodiscard]]
	inline const std::string& get_component() const& noexcept;
	[[nodiscard]]
	inline std::string& get_component() & noexcept;
	[[nodiscard]]
	inline std::string get_component() && noexcept;

	[[nodiscard]]
	inline const std::optional<std::string>& get_version() const& noexcept;
	[[nodiscard]]
	inline std::optional<std::string>& get_version() & noexcept;
	[[nodiscard]]
	inline std::optional<std::string> get_version() && noexcept;

	[[nodiscard]]
	bool is_valid() const noexcept;

	[[nodiscard]]
	static com_progid parse(std::string_view str);

	[[nodiscard]]
	friend bool operator==(const com_progid&, const com_progid&) noexcept = default;

private:
	std::string program_;
	std::string component_;
	std::optional<std::string> version_;
};

// https://learn.microsoft.com/en-us/windows/win32/api/oleidl/ne-oleidl-olemisc
struct ole_misc final
{
	enum value : std::uint32_t
	{
		recomposeonresize = 1u << 0u,
		onlyiconic = 1u << 1u,
		insertnotreplace = 1u << 2u,
		static_value = 1u << 3u,
		cantlinkinside = 1u << 4u,
		canlinkbyole1 = 1u << 5u,
		islinkobject = 1u << 6u,
		insideout = 1u << 7u,
		activatewhenvisible = 1u << 8u,
		renderingisdeviceindependent = 1u << 9u,
		invisibleatruntime = 1u << 10u,
		alwaysrun = 1u << 11u,
		actslikebutton = 1u << 12u,
		actslikelabel = 1u << 13u,
		nouiactivate = 1u << 14u,
		alignable = 1u << 15u,
		simpleframe = 1u << 16u,
		setclientsitefirst = 1u << 17u,
		imemode = 1u << 18u,
		ignoreativatewhenvisible = 1u << 19u,
		wantstomenumerge = 1u << 20u,
		supportsmultilevelundo = 1u << 21u,
		unknown = 1u << 22u
	};
};

// https://learn.microsoft.com/en-us/windows/win32/sbscs/assembly-manifests
template<typename... Bases>
class [[nodiscard]] com_class_base : public Bases...
{
public:
	[[nodiscard]]
	const std::string& get_clsid_raw() const& noexcept;
	[[nodiscard]]
	std::string& get_clsid_raw() & noexcept;
	[[nodiscard]]
	std::string get_clsid_raw() && noexcept;
	[[nodiscard]]
	guid get_clsid() const;

	[[nodiscard]]
	const std::optional<std::string>& get_tlbid_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_tlbid_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_tlbid_raw() && noexcept;
	[[nodiscard]]
	std::optional<guid> get_tlbid() const;

	[[nodiscard]]
	const std::optional<std::string>& get_progid_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_progid_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_progid_raw() && noexcept;
	[[nodiscard]]
	std::optional<com_progid> get_progid() const;

	[[nodiscard]]
	const std::optional<std::string>& get_description() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_description() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_description() && noexcept;

	[[nodiscard]]
	const std::optional<std::string>&
		get_threading_model_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>&
		get_threading_model_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string>
		get_threading_model_raw() && noexcept;
	[[nodiscard]]
	com_threading_model get_threading_model() const noexcept;

	[[nodiscard]]
	const std::vector<std::string>& get_additional_progids() const& noexcept;
	[[nodiscard]]
	std::vector<std::string>& get_additional_progids() & noexcept;
	[[nodiscard]]
	std::vector<std::string> get_additional_progids() && noexcept;

public:
	[[nodiscard]]
	const std::optional<std::string>& get_misc_status_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_misc_status_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_misc_status_raw() && noexcept;
	[[nodiscard]]
	ole_misc::value get_misc_status() const;

	[[nodiscard]]
	const std::optional<std::string>& get_misc_status_icon_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_misc_status_icon_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_misc_status_icon_raw() && noexcept;
	[[nodiscard]]
	ole_misc::value get_misc_status_icon() const;

	[[nodiscard]]
	const std::optional<std::string>& get_misc_status_content_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_misc_status_content_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_misc_status_content_raw() && noexcept;
	[[nodiscard]]
	ole_misc::value get_misc_status_content() const;

	[[nodiscard]]
	const std::optional<std::string>& get_misc_status_docprint_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_misc_status_docprint_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_misc_status_docprint_raw() && noexcept;
	[[nodiscard]]
	ole_misc::value get_misc_status_docprint() const;

	[[nodiscard]]
	const std::optional<std::string>& get_misc_status_thumbnail_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_misc_status_thumbnail_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_misc_status_thumbnail_raw() && noexcept;
	[[nodiscard]]
	ole_misc::value get_misc_status_thumbnail() const;

private:
	std::string clsid_;
	std::optional<std::string> tlbid_;
	std::optional<std::string> progid_;
	std::optional<std::string> description_;
	std::optional<std::string> threading_model_;
	std::optional<std::string> misc_status_;
	std::optional<std::string> misc_status_icon_;
	std::optional<std::string> misc_status_content_;
	std::optional<std::string> misc_status_docprint_;
	std::optional<std::string> misc_status_thumbnail_;
	std::vector<std::string> additional_progids_;
};

// https://learn.microsoft.com/en-us/windows/win32/api/oaidl/ne-oaidl-libflags
struct com_typelib_flags final
{
	enum value : std::uint32_t
	{
		restricted = 1u << 0u,
		control = 1u << 1u,
		hidden = 1u << 2u,
		has_disk_image = 1u << 3u,
		unknown = 1u << 4u
	};
};

template<typename... Bases>
class [[nodiscard]] com_typelib_base : public Bases...
{
public:
	[[nodiscard]]
	const std::string& get_tlbid_raw() const& noexcept;
	[[nodiscard]]
	std::string& get_tlbid_raw() & noexcept;
	[[nodiscard]]
	std::string get_tlbid_raw() && noexcept;
	[[nodiscard]]
	guid get_tlbid() const;

	[[nodiscard]]
	const std::string& get_version_raw() const& noexcept;
	[[nodiscard]]
	std::string& get_version_raw() & noexcept;
	[[nodiscard]]
	std::string get_version_raw() && noexcept;
	[[nodiscard]]
	short_version get_version() const;

	[[nodiscard]]
	const std::string& get_help_dir() const& noexcept;
	[[nodiscard]]
	std::string& get_help_dir() & noexcept;
	[[nodiscard]]
	std::string get_help_dir() && noexcept;

	[[nodiscard]]
	const std::optional<std::string>& get_resource_id_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_resource_id_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_resource_id_raw() && noexcept;
	[[nodiscard]]
	std::optional<lcid_type> get_resource_id() const;

	[[nodiscard]]
	const std::optional<std::string>& get_flags_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_flags_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_flags_raw() && noexcept;
	[[nodiscard]]
	std::optional<com_typelib_flags::value> get_flags() const;

private:
	std::string tlbid_;
	std::string version_;
	std::string help_dir_;
	std::optional<std::string> resource_id_;
	std::optional<std::string> flags_;
};

template<typename... Bases>
class [[nodiscard]] com_interface_external_proxy_stub_base : public Bases...
{
public:
	[[nodiscard]]
	const std::string& get_iid_raw() const& noexcept;
	[[nodiscard]]
	std::string& get_iid_raw() & noexcept;
	[[nodiscard]]
	std::string get_iid_raw() && noexcept;
	[[nodiscard]]
	guid get_iid() const;

	[[nodiscard]]
	const std::optional<std::string>& get_base_interface_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_base_interface_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_base_interface_raw() && noexcept;
	[[nodiscard]]
	std::optional<guid> get_base_interface() const;

	[[nodiscard]]
	const std::optional<std::string>& get_name() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_name() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_name() && noexcept;

	[[nodiscard]]
	const std::optional<std::string>& get_tlbid_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_tlbid_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_tlbid_raw() && noexcept;
	[[nodiscard]]
	std::optional<guid> get_tlbid() const;

	[[nodiscard]]
	const std::optional<std::string>& get_proxy_stub_clsid32_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_proxy_stub_clsid32_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_proxy_stub_clsid32_raw() && noexcept;
	[[nodiscard]]
	std::optional<guid> get_proxy_stub_clsid32() const;

	[[nodiscard]]
	const std::optional<std::string>& get_num_methods_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_num_methods_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_num_methods_raw() && noexcept;
	[[nodiscard]]
	std::optional<std::uint64_t> get_num_methods() const;

private:
	std::string iid_;
	std::optional<std::string> base_interface_;
	std::optional<std::string> num_methods_;
	std::optional<std::string> name_;
	std::optional<std::string> tlbid_;
	std::optional<std::string> proxy_stub_clsid32_;
};

template<typename... Bases>
class [[nodiscard]] com_interface_proxy_stub_base
	: public com_interface_external_proxy_stub_base<Bases...>
{
public:
	[[nodiscard]]
	const std::optional<std::string>& get_threading_model_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_threading_model_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_threading_model_raw() && noexcept;
	[[nodiscard]]
	com_threading_model get_threading_model() const noexcept;

private:
	std::optional<std::string> threading_model_;
};

template<typename... Bases>
class [[nodiscard]] window_class_base : public Bases...
{
public:
	[[nodiscard]]
	const std::string& get_class() const& noexcept;
	[[nodiscard]]
	std::string& get_class() & noexcept;
	[[nodiscard]]
	std::string get_class() && noexcept;

	[[nodiscard]]
	const std::optional<std::string>& is_versioned_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& is_versioned_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> is_versioned_raw() && noexcept;
	[[nodiscard]]
	bool is_versioned() const;

private:
	std::string class_;
	std::optional<std::string> versioned_;
};

enum class assembly_file_hash_algorithm
{
	sha1,
	sha256,
	unspecified,
	unknown
};

template<typename... Bases>
class [[nodiscard]] assembly_file_base : public Bases...
{
public:
	using com_class_type = com_class_base<Bases...>;
	using com_typelib_type = com_typelib_base<Bases...>;
	using com_interface_proxy_stub_type = com_interface_proxy_stub_base<Bases...>;
	using window_class_type = window_class_base<Bases...>;

public:
	[[nodiscard]]
	const std::string& get_name() const& noexcept;
	[[nodiscard]]
	std::string& get_name() & noexcept;
	[[nodiscard]]
	std::string get_name() && noexcept;

	[[nodiscard]]
	const std::optional<std::string>& get_hash_algorithm_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_hash_algorithm_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_hash_algorithm_raw() && noexcept;
	[[nodiscard]]
	assembly_file_hash_algorithm get_hash_algorithm() const noexcept;

	[[nodiscard]]
	const std::optional<std::string>& get_hash_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_hash_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_hash_raw() && noexcept;
	[[nodiscard]]
	std::optional<std::vector<std::byte>> get_hash() const;

	[[nodiscard]]
	const std::vector<com_class_type>& get_com_classes() const& noexcept;
	[[nodiscard]]
	std::vector<com_class_type>& get_com_classes() & noexcept;
	[[nodiscard]]
	std::vector<com_class_type> get_com_classes() && noexcept;

	[[nodiscard]]
	const std::vector<com_typelib_type>& get_com_typelibs() const& noexcept;
	[[nodiscard]]
	std::vector<com_typelib_type>& get_com_typelibs() & noexcept;
	[[nodiscard]]
	std::vector<com_typelib_type> get_com_typelibs() && noexcept;

	[[nodiscard]]
	const std::vector<com_interface_proxy_stub_type>&
		get_com_interface_proxy_stubs() const& noexcept;
	[[nodiscard]]
	std::vector<com_interface_proxy_stub_type>&
		get_com_interface_proxy_stubs() & noexcept;
	[[nodiscard]]
	std::vector<com_interface_proxy_stub_type>
		get_com_interface_proxy_stubs() && noexcept;

	[[nodiscard]]
	const std::vector<window_class_type>& get_window_classes() const& noexcept;
	[[nodiscard]]
	std::vector<window_class_type>& get_window_classes() & noexcept;
	[[nodiscard]]
	std::vector<window_class_type> get_window_classes() && noexcept;

	[[nodiscard]]
	const std::optional<std::string>& get_size_raw() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_size_raw() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_size_raw() && noexcept;
	[[nodiscard]]
	std::optional<std::uint64_t> get_size() const;

private:
	std::string name_;
	std::optional<std::string> hash_algorithm_;
	std::optional<std::string> hash_;
	std::optional<std::string> size_;
	std::vector<com_class_type> com_classes_;
	std::vector<com_typelib_type> com_typelibs_;
	std::vector<com_interface_proxy_stub_type> com_interface_proxy_stubs_;
	std::vector<window_class_type> window_classes_;
};

struct legacy_code_page_tag {};
struct utf8_code_page_tag {};

using active_code_page_type = std::variant<std::monostate,
	legacy_code_page_tag, utf8_code_page_tag, lcid_info>;

class [[nodiscard]] active_code_page
{
public:
	[[nodiscard]]
	inline const std::string& get_name() const& noexcept;
	[[nodiscard]]
	inline std::string& get_name() & noexcept;
	[[nodiscard]]
	inline std::string get_name() && noexcept;

	[[nodiscard]]
	active_code_page_type get_code_page() const noexcept;

private:
	std::string name_;
};

enum class dpi_aware_value
{
	//The current process is dpi unaware by default.
	//You can programmatically change this setting by calling
	//the SetProcessDpiAwareness or SetProcessDPIAware function.
	absent,
	//The current process is system dpi aware.
	dpi_aware,
	//Windows Vista, Windows 7 and Windows 8:
	// The behavior is the same as when the dpiAware is absent.
	//Windows 8.1 and Windows 10:
	// The current process is dpi unaware, and you cannot
	// programmatically change this setting by calling
	// the SetProcessDpiAwareness or SetProcessDPIAware function.
	dpi_unaware,
	//Windows Vista, Windows 7 and Windows 8:
	// The current process is system dpi aware.
	//Windows 8.1 and Windows 10:
	// The current process is per - monitor dpi aware.
	dpi_aware_true_per_monitor,
	//Windows Vista, Windows 7 and Windows 8:
	// The behavior is the same as when the dpiAware is absent.
	//Windows 8.1 and Windows 10:
	// The current process is per - monitor dpi aware.
	dpi_aware_per_monitor,
};

//Windows 10, version 1607 or newer
enum class dpi_awareness_value
{
	//The dpiAware element specifies whether the process is dpi aware.
	absent,
	//The current process is dpi unaware by default.
	//You can programmatically change this setting by calling
	//the SetProcessDpiAwareness or SetProcessDPIAware function.
	unrecognized,
	//The current process is system dpi aware.
	dpi_aware_system,
	//The current process is per-monitor dpi aware.
	dpi_aware_per_monitor,
	//The current process uses the per-monitor-v2 dpi awareness context.
	//This item will only be recognized on Windows 10 version 1703 or later.
	dpi_aware_per_monitor_v2,
	//The current process is dpi unaware. You cannot programmatically
	//change this setting by calling the SetProcessDpiAwareness or SetProcessDPIAware function.
	dpi_unaware
};

class [[nodiscard]] dpi_awareness
{
public:
	[[nodiscard]]
	inline const std::optional<std::string>& get_dpi_aware_raw() const& noexcept;
	[[nodiscard]]
	inline std::optional<std::string>& get_dpi_aware_raw() & noexcept;
	[[nodiscard]]
	inline std::optional<std::string> get_dpi_aware_raw() && noexcept;
	[[nodiscard]]
	dpi_aware_value get_dpi_aware_value() const noexcept;

	[[nodiscard]]
	inline const std::optional<std::string>& get_dpi_awareness_raw() const& noexcept;
	[[nodiscard]]
	inline std::optional<std::string>& get_dpi_awareness_raw() & noexcept;
	[[nodiscard]]
	inline std::optional<std::string> get_dpi_awareness_raw() && noexcept;
	[[nodiscard]]
	dpi_awareness_value get_dpi_awareness_value() const noexcept;

private:
	std::optional<std::string> dpi_aware_;
	std::optional<std::string> dpi_awareness_;
};

enum class heap_type_value
{
	unknown,
	segment_heap
};

class [[nodiscard]] heap_type
{
public:
	[[nodiscard]]
	inline const std::string& get_heap_type_raw() const& noexcept;
	[[nodiscard]]
	inline std::string& get_heap_type_raw() & noexcept;
	[[nodiscard]]
	inline std::string get_heap_type_raw() && noexcept;
	[[nodiscard]]
	heap_type_value get_heap_type() const noexcept;

private:
	std::string heap_type_;
};

enum class requested_execution_level
{
	unknown,
	as_invoker,
	require_administrator,
	highest_available
};

class [[nodiscard]] requested_privileges
{
public:
	[[nodiscard]]
	inline const std::string& get_level_raw() const& noexcept;
	[[nodiscard]]
	inline std::string& get_level_raw() & noexcept;
	[[nodiscard]]
	inline std::string get_level_raw() && noexcept;
	[[nodiscard]]
	requested_execution_level get_level() const noexcept;

	[[nodiscard]]
	inline const std::string& get_ui_access_raw() const& noexcept;
	[[nodiscard]]
	inline std::string& get_ui_access_raw() & noexcept;
	[[nodiscard]]
	inline std::string get_ui_access_raw() && noexcept;
	[[nodiscard]]
	std::optional<bool> get_ui_access() const noexcept;

private:
	std::string level_;
	std::string ui_access_;
};

template<typename... Bases>
class [[nodiscard]] msix_identity_base : public Bases...
{
public:
	[[nodiscard]]
	const std::string& get_publisher() const& noexcept;
	[[nodiscard]]
	std::string& get_publisher() & noexcept;
	[[nodiscard]]
	std::string get_publisher() && noexcept;

	[[nodiscard]]
	const std::string& get_package_name() const& noexcept;
	[[nodiscard]]
	std::string& get_package_name() & noexcept;
	[[nodiscard]]
	std::string get_package_name() && noexcept;

	[[nodiscard]]
	const std::string& get_application_id() const& noexcept;
	[[nodiscard]]
	std::string& get_application_id() & noexcept;
	[[nodiscard]]
	std::string get_application_id() && noexcept;

private:
	std::string publisher_;
	std::string package_name_;
	std::string application_id_;
};

enum class assembly_no_inherit
{
	absent,
	no_inherit,
	no_inheritable
};

template<typename... Bases>
class [[nodiscard]] native_manifest_base : public Bases...
{
public:
	using assembly_file_type = assembly_file_base<Bases...>;
	using com_interface_external_proxy_stub_type
		= com_interface_external_proxy_stub_base<Bases...>;
	using msix_identity_type = msix_identity_base<Bases...>;

	static constexpr std::string_view supported_manifest_version = "1.0";

public:
	[[nodiscard]]
	const std::string& get_manifest_version() const& noexcept;
	[[nodiscard]]
	std::string& get_manifest_version() & noexcept;
	[[nodiscard]]
	std::string get_manifest_version() && noexcept;

	[[nodiscard]]
	assembly_no_inherit no_inherit() const noexcept;

	void set_no_inherit(assembly_no_inherit value) noexcept;

	[[nodiscard]]
	const assembly_identity_base<Bases...>& get_assembly_identity() const& noexcept;
	[[nodiscard]]
	assembly_identity_base<Bases...>& get_assembly_identity() & noexcept;
	[[nodiscard]]
	assembly_identity_base<Bases...> get_assembly_identity() && noexcept;

	[[nodiscard]]
	const std::optional<assembly_supported_os_list_base<Bases...>>&
		get_supported_os_list() const& noexcept;
	[[nodiscard]]
	std::optional<assembly_supported_os_list_base<Bases...>>&
		get_supported_os_list() & noexcept;
	[[nodiscard]]
	std::optional<assembly_supported_os_list_base<Bases...>>
		get_supported_os_list() && noexcept;

	[[nodiscard]]
	const std::vector<assembly_identity_base<Bases...>>&
		get_dependencies() const& noexcept;
	[[nodiscard]]
	std::vector<assembly_identity_base<Bases...>>&
		get_dependencies() & noexcept;
	[[nodiscard]]
	std::vector<assembly_identity_base<Bases...>>
		get_dependencies() && noexcept;

	[[nodiscard]]
	const std::vector<assembly_file_type>& get_files() const& noexcept;
	[[nodiscard]]
	std::vector<assembly_file_type>& get_files() & noexcept;
	[[nodiscard]]
	std::vector<assembly_file_type> get_files() && noexcept;

	[[nodiscard]]
	const std::optional<active_code_page>& get_active_code_page() const& noexcept;
	[[nodiscard]]
	std::optional<active_code_page>& get_active_code_page() & noexcept;
	[[nodiscard]]
	std::optional<active_code_page> get_active_code_page() && noexcept;

	[[nodiscard]]
	const std::optional<std::string>& get_description() const& noexcept;
	[[nodiscard]]
	std::optional<std::string>& get_description() & noexcept;
	[[nodiscard]]
	std::optional<std::string> get_description() && noexcept;

	[[nodiscard]]
	const dpi_awareness& get_dpi_awareness() const& noexcept;
	[[nodiscard]]
	dpi_awareness& get_dpi_awareness() & noexcept;
	[[nodiscard]]
	dpi_awareness get_dpi_awareness() && noexcept;

	[[nodiscard]]
	bool auto_elevate() const noexcept;

	[[nodiscard]]
	bool disable_theming() const noexcept;

	[[nodiscard]]
	bool disable_window_filtering() const noexcept;

	[[nodiscard]]
	bool gdi_scaling() const noexcept;

	[[nodiscard]]
	bool high_resolution_scrolling_aware() const noexcept;

	[[nodiscard]]
	bool long_path_aware() const noexcept;

	[[nodiscard]]
	bool printer_driver_isolation() const noexcept;

	[[nodiscard]]
	bool ultra_high_resolution_scrolling_aware() const noexcept;

	void set_auto_elevate(bool value) noexcept;
	void set_disable_theming(bool value) noexcept;
	void set_disable_window_filtering(bool value) noexcept;
	void set_gdi_scaling(bool value) noexcept;
	void set_high_resolution_scrolling_aware(bool value) noexcept;
	void set_long_path_aware(bool value) noexcept;
	void set_printer_driver_isolation(bool value) noexcept;
	void set_ultra_high_resolution_scrolling_aware(bool value) noexcept;

	[[nodiscard]]
	const std::optional<heap_type>& get_heap_type() const& noexcept;
	[[nodiscard]]
	std::optional<heap_type>& get_heap_type() & noexcept;
	[[nodiscard]]
	std::optional<heap_type> get_heap_type() && noexcept;

	[[nodiscard]]
	const std::optional<requested_privileges>&
		get_requested_privileges() const& noexcept;
	[[nodiscard]]
	std::optional<requested_privileges>&
		get_requested_privileges() & noexcept;
	[[nodiscard]]
	std::optional<requested_privileges>
		get_requested_privileges() && noexcept;

	[[nodiscard]]
	const std::vector<com_interface_external_proxy_stub_type>&
		get_com_interface_external_proxy_stubs() const& noexcept;
	[[nodiscard]]
	std::vector<com_interface_external_proxy_stub_type>&
		get_com_interface_external_proxy_stubs() & noexcept;
	[[nodiscard]]
	std::vector<com_interface_external_proxy_stub_type>
		get_com_interface_external_proxy_stubs() && noexcept;

	[[nodiscard]]
	const std::optional<msix_identity_type>&
		get_msix_identity() const& noexcept;
	[[nodiscard]]
	std::optional<msix_identity_type>&
		get_msix_identity() & noexcept;
	[[nodiscard]]
	std::optional<msix_identity_type>
		get_msix_identity() && noexcept;

private:
	std::string manifest_version_;
	assembly_no_inherit no_inherit_ = assembly_no_inherit::absent;
	assembly_identity_base<Bases...> assembly_identity_;
	std::optional<assembly_supported_os_list_base<Bases...>> supported_os_list_;
	std::vector<assembly_identity_base<Bases...>> dependencies_;
	std::vector<assembly_file_type> files_;
	std::optional<active_code_page> active_code_page_;
	std::optional<std::string> description_;
	bool auto_elevate_{};
	bool disable_theming_{};
	bool disable_window_filtering_{};
	dpi_awareness dpi_awareness_;
	bool gdi_scaling_{};
	bool high_resolution_scrolling_aware_{};
	bool long_path_aware_{};
	bool printer_driver_isolation_{};
	bool ultra_high_resolution_scrolling_aware_{};
	std::optional<heap_type> heap_type_;
	std::optional<requested_privileges> requested_privileges_;
	std::vector<com_interface_external_proxy_stub_type>
		com_interface_external_proxy_stubs_;
	std::optional<msix_identity_type> msix_identity_;
};

[[nodiscard]]
full_version parse_full_version(std::string_view version_string);
[[nodiscard]]
short_version parse_short_version(std::string_view version_string);

using native_manifest = native_manifest_base<>;
using native_manifest_details = native_manifest_base<error_list>;
using assembly_identity = assembly_identity_base<>;
using assembly_identity_details = assembly_identity_base<error_list>;
using assembly_supported_os_list = assembly_supported_os_list_base<>;
using assembly_supported_os_list_details = assembly_supported_os_list_base<error_list>;
using assembly_file = assembly_file_base<>;
using assembly_file_details = assembly_file_base<error_list>;
using com_class = com_class_base<>;
using com_class_details = com_class_base<error_list>;
using com_typelib = com_typelib_base<>;
using com_typelib_details = com_typelib_base<error_list>;
using com_interface_external_proxy_stub
	= com_interface_external_proxy_stub_base<>;
using com_interface_external_proxy_stub_details
	= com_interface_external_proxy_stub_base<error_list>;
using com_interface_proxy_stub = com_interface_proxy_stub_base<>;
using com_interface_proxy_stub_details = com_interface_proxy_stub_base<error_list>;
using window_class = window_class_base<>;
using window_class_details = window_class_base<error_list>;
using msix_identity = msix_identity_base<>;
using msix_identity_details = msix_identity_base<error_list>;

class manifest_accessor_interface;

[[nodiscard]]
native_manifest_details parse_manifest(const manifest_accessor_interface& accessor);

} //namespace pe_bliss::resources

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::resources::manifest_errc> : true_type {};
} //namespace std

#include "pe_bliss2/resources/manifest-inl.h"
