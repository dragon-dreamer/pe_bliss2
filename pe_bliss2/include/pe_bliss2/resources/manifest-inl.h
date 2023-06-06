#include <utility>

namespace pe_bliss::resources
{

template<typename... Bases>
const std::string& assembly_identity_base<Bases...>
	::get_type_raw() const& noexcept
{
	return type_;
}

template<typename... Bases>
std::string& assembly_identity_base<Bases...>
	::get_type_raw() & noexcept
{
	return type_;
}

template<typename... Bases>
std::string assembly_identity_base<Bases...>
	::get_type_raw() && noexcept
{
	return std::move(type_);
}

template<typename... Bases>
const std::string& assembly_identity_base<Bases...>
	::get_name() const& noexcept
{
	return name_;
}

template<typename... Bases>
std::string& assembly_identity_base<Bases...>
	::get_name() & noexcept
{
	return name_;
}

template<typename... Bases>
std::string assembly_identity_base<Bases...>
	::get_name() && noexcept
{
	return std::move(name_);
}

template<typename... Bases>
const std::optional<std::string>& assembly_identity_base<Bases...>
	::get_language_raw() const& noexcept
{
	return language_;
}

template<typename... Bases>
std::optional<std::string>& assembly_identity_base<Bases...>
	::get_language_raw() & noexcept
{
	return language_;
}

template<typename... Bases>
std::optional<std::string> assembly_identity_base<Bases...>
	::get_language_raw() && noexcept
{
	return std::move(language_);
}

template<typename... Bases>
const std::optional<std::string>& assembly_identity_base<Bases...>
	::get_processor_architecture_raw() const& noexcept
{
	return processor_architecture_;
}

template<typename... Bases>
std::optional<std::string>& assembly_identity_base<Bases...>
	::get_processor_architecture_raw() & noexcept
{
	return processor_architecture_;
}

template<typename... Bases>
std::optional<std::string> assembly_identity_base<Bases...>
	::get_processor_architecture_raw() && noexcept
{
	return std::move(processor_architecture_);
}

template<typename... Bases>
const std::string& assembly_identity_base<Bases...>
	::get_version_raw() const& noexcept
{
	return version_;
}

template<typename... Bases>
std::string& assembly_identity_base<Bases...>
	::get_version_raw() & noexcept
{
	return version_;
}

template<typename... Bases>
std::string assembly_identity_base<Bases...>
	::get_version_raw() && noexcept
{
	return std::move(version_);
}

template<typename... Bases>
const std::optional<std::string>& assembly_identity_base<Bases...>
	::get_public_key_token_raw() const& noexcept
{
	return public_key_token_;
}

template<typename... Bases>
std::optional<std::string>& assembly_identity_base<Bases...>
	::get_public_key_token_raw() & noexcept
{
	return public_key_token_;
}

template<typename... Bases>
std::optional<std::string> assembly_identity_base<Bases...>
	::get_public_key_token_raw() && noexcept
{
	return public_key_token_;
}

template<typename... Bases>
const std::string& assembly_file_base<Bases...>::get_name() const& noexcept
{
	return name_;
}

template<typename... Bases>
std::string& assembly_file_base<Bases...>::get_name() & noexcept
{
	return name_;
}

template<typename... Bases>
std::string assembly_file_base<Bases...>::get_name() && noexcept
{
	return std::move(name_);
}

template<typename... Bases>
const std::optional<std::string>& assembly_file_base<Bases...>
	::get_hash_algorithm_raw() const& noexcept
{
	return hash_algorithm_;
}

template<typename... Bases>
std::optional<std::string>& assembly_file_base<Bases...>
	::get_hash_algorithm_raw() & noexcept
{
	return hash_algorithm_;
}

template<typename... Bases>
std::optional<std::string> assembly_file_base<Bases...>
	::get_hash_algorithm_raw() && noexcept
{
	return std::move(hash_algorithm_);
}

template<typename... Bases>
const std::optional<std::string>& assembly_file_base<Bases...>
	::get_hash_raw() const& noexcept
{
	return hash_;
}

template<typename... Bases>
std::optional<std::string>& assembly_file_base<Bases...>
	::get_hash_raw() & noexcept
{
	return hash_;
}

template<typename... Bases>
std::optional<std::string> assembly_file_base<Bases...>
	::get_hash_raw() && noexcept
{
	return std::move(hash_);
}

template<typename... Bases>
const std::vector<typename assembly_file_base<Bases...>
	::com_class_type>& assembly_file_base<Bases...>
	::get_com_classes() const& noexcept
{
	return com_classes_;
}

template<typename... Bases>
std::vector<typename assembly_file_base<Bases...>
	::com_class_type>& assembly_file_base<Bases...>
	::get_com_classes() & noexcept
{
	return com_classes_;
}

template<typename... Bases>
std::vector<typename assembly_file_base<Bases...>
	::com_class_type> assembly_file_base<Bases...>
	::get_com_classes() && noexcept
{
	return std::move(com_classes_);
}

template<typename... Bases>
const std::vector<typename assembly_file_base<Bases...>
	::com_typelib_type>& assembly_file_base<Bases...>
	::get_com_typelibs() const& noexcept
{
	return com_typelibs_;
}

template<typename... Bases>
std::vector<typename assembly_file_base<Bases...>
	::com_typelib_type>& assembly_file_base<Bases...>
	::get_com_typelibs() & noexcept
{
	return com_typelibs_;
}

template<typename... Bases>
std::vector<typename assembly_file_base<Bases...>
	::com_typelib_type> assembly_file_base<Bases...>
	::get_com_typelibs() && noexcept
{
	return std::move(com_typelibs_);
}

template<typename... Bases>
const std::vector<typename assembly_file_base<Bases...>
	::com_interface_proxy_stub_type>& assembly_file_base<Bases...>
	::get_com_interface_proxy_stubs() const& noexcept
{
	return com_interface_proxy_stubs_;
}

template<typename... Bases>
std::vector<typename assembly_file_base<Bases...>
	::com_interface_proxy_stub_type>& assembly_file_base<Bases...>
	::get_com_interface_proxy_stubs() & noexcept
{
	return com_interface_proxy_stubs_;
}

template<typename... Bases>
std::vector<typename assembly_file_base<Bases...>
	::com_interface_proxy_stub_type> assembly_file_base<Bases...>
	::get_com_interface_proxy_stubs() && noexcept
{
	return std::move(com_interface_proxy_stubs_);
}

template<typename... Bases>
const std::vector<typename assembly_file_base<Bases...>
	::window_class_type>& assembly_file_base<Bases...>
	::get_window_classes() const& noexcept
{
	return window_classes_;
}

template<typename... Bases>
std::vector<typename assembly_file_base<Bases...>
	::window_class_type>& assembly_file_base<Bases...>
	::get_window_classes() & noexcept
{
	return window_classes_;
}

template<typename... Bases>
std::vector<typename assembly_file_base<Bases...>
	::window_class_type> assembly_file_base<Bases...>
	::get_window_classes() && noexcept
{
	return std::move(window_classes_);
}

template<typename... Bases>
const std::optional<std::string>& assembly_file_base<Bases...>
	::get_size_raw() const& noexcept
{
	return size_;
}

template<typename... Bases>
std::optional<std::string>& assembly_file_base<Bases...>
	::get_size_raw() & noexcept
{
	return size_;
}

template<typename... Bases>
std::optional<std::string> assembly_file_base<Bases...>
	::get_size_raw() && noexcept
{
	return size_;
}

inline const std::string& active_code_page::get_name() const& noexcept
{
	return name_;
}

inline std::string& active_code_page::get_name() & noexcept
{
	return name_;
}

inline std::string active_code_page::get_name() && noexcept
{
	return std::move(name_);
}

inline const std::optional<std::string>& dpi_awareness::get_dpi_aware_raw() const& noexcept
{
	return dpi_aware_;
}

inline std::optional<std::string>& dpi_awareness::get_dpi_aware_raw() & noexcept
{
	return dpi_aware_;
}

inline std::optional<std::string> dpi_awareness::get_dpi_aware_raw() && noexcept
{
	return std::move(dpi_aware_);
}

inline const std::optional<std::string>& dpi_awareness
	::get_dpi_awareness_raw() const& noexcept
{
	return dpi_awareness_;
}

inline std::optional<std::string>& dpi_awareness
	::get_dpi_awareness_raw() & noexcept
{
	return dpi_awareness_;
}

inline std::optional<std::string> dpi_awareness
	::get_dpi_awareness_raw() && noexcept
{
	return std::move(dpi_awareness_);
}

inline const std::string& heap_type::get_heap_type_raw() const& noexcept
{
	return heap_type_;
}

inline std::string& heap_type::get_heap_type_raw() & noexcept
{
	return heap_type_;
}

inline std::string heap_type::get_heap_type_raw() && noexcept
{
	return std::move(heap_type_);
}

inline const std::string& requested_privileges::get_level_raw() const& noexcept
{
	return level_;
}

inline std::string& requested_privileges::get_level_raw() & noexcept
{
	return level_;
}

inline std::string requested_privileges::get_level_raw() && noexcept
{
	return std::move(level_);
}

inline const std::string& requested_privileges::get_ui_access_raw() const& noexcept
{
	return ui_access_;
}

inline std::string& requested_privileges::get_ui_access_raw() & noexcept
{
	return ui_access_;
}

inline std::string requested_privileges::get_ui_access_raw() && noexcept
{
	return std::move(ui_access_);
}

template<typename... Bases>
const std::string& com_class_base<Bases...>::get_clsid_raw() const& noexcept
{
	return clsid_;
}

template<typename... Bases>
std::string& com_class_base<Bases...>::get_clsid_raw() & noexcept
{
	return clsid_;
}

template<typename... Bases>
std::string com_class_base<Bases...>::get_clsid_raw() && noexcept
{
	return std::move(clsid_);
}

template<typename... Bases>
const std::optional<std::string>& com_class_base<Bases...>::get_tlbid_raw() const& noexcept
{
	return tlbid_;
}

template<typename... Bases>
std::optional<std::string>& com_class_base<Bases...>::get_tlbid_raw() & noexcept
{
	return tlbid_;
}

template<typename... Bases>
std::optional<std::string> com_class_base<Bases...>::get_tlbid_raw() && noexcept
{
	return std::move(tlbid_);
}

template<typename... Bases>
const std::optional<std::string>& com_class_base<Bases...>::get_description() const& noexcept
{
	return description_;
}

template<typename... Bases>
std::optional<std::string>& com_class_base<Bases...>::get_description() & noexcept
{
	return description_;
}

template<typename... Bases>
std::optional<std::string> com_class_base<Bases...>::get_description() && noexcept
{
	return std::move(description_);
}

template<typename... Bases>
const std::optional<std::string>&
	com_class_base<Bases...>::get_threading_model_raw() const& noexcept
{
	return threading_model_;
}

template<typename... Bases>
std::optional<std::string>&
	com_class_base<Bases...>::get_threading_model_raw() & noexcept
{
	return threading_model_;
}

template<typename... Bases>
std::optional<std::string>
	com_class_base<Bases...>::get_threading_model_raw() && noexcept
{
	return std::move(threading_model_);
}

template<typename... Bases>
const std::optional<std::string>& com_class_base<Bases...>
	::get_progid_raw() const& noexcept
{
	return progid_;
}

template<typename... Bases>
std::optional<std::string>& com_class_base<Bases...>
	::get_progid_raw() & noexcept
{
	return progid_;
}

template<typename... Bases>
std::optional<std::string> com_class_base<Bases...>
	::get_progid_raw() && noexcept
{
	return std::move(progid_);
}

template<typename... Bases>
const std::optional<std::string>& com_class_base<Bases...>
	::get_misc_status_raw() const& noexcept
{
	return misc_status_;
}

template<typename... Bases>
std::optional<std::string>& com_class_base<Bases...>
	::get_misc_status_raw() & noexcept
{
	return misc_status_;
}

template<typename... Bases>
std::optional<std::string> com_class_base<Bases...>
	::get_misc_status_raw() && noexcept
{
	return std::move(misc_status_);
}

template<typename... Bases>
const std::optional<std::string>& com_class_base<Bases...>
	::get_misc_status_icon_raw() const& noexcept
{
	return misc_status_icon_;
}

template<typename... Bases>
std::optional<std::string>& com_class_base<Bases...>
	::get_misc_status_icon_raw() & noexcept
{
	return misc_status_icon_;
}

template<typename... Bases>
std::optional<std::string> com_class_base<Bases...>
	::get_misc_status_icon_raw() && noexcept
{
	return std::move(misc_status_icon_);
}

template<typename... Bases>
const std::optional<std::string>& com_class_base<Bases...>
	::get_misc_status_content_raw() const& noexcept
{
	return misc_status_content_;
}

template<typename... Bases>
std::optional<std::string>& com_class_base<Bases...>
	::get_misc_status_content_raw() & noexcept
{
	return misc_status_content_;
}

template<typename... Bases>
std::optional<std::string> com_class_base<Bases...>
	::get_misc_status_content_raw() && noexcept
{
	return std::move(misc_status_content_);
}

template<typename... Bases>
const std::optional<std::string>& com_class_base<Bases...>
	::get_misc_status_docprint_raw() const& noexcept
{
	return misc_status_docprint_;
}

template<typename... Bases>
std::optional<std::string>& com_class_base<Bases...>
	::get_misc_status_docprint_raw() & noexcept
{
	return misc_status_docprint_;
}

template<typename... Bases>
std::optional<std::string> com_class_base<Bases...>
	::get_misc_status_docprint_raw() && noexcept
{
	return std::move(misc_status_docprint_);
}

template<typename... Bases>
const std::optional<std::string>& com_class_base<Bases...>
	::get_misc_status_thumbnail_raw() const& noexcept
{
	return misc_status_thumbnail_;
}

template<typename... Bases>
std::optional<std::string>& com_class_base<Bases...>
	::get_misc_status_thumbnail_raw() & noexcept
{
	return misc_status_thumbnail_;
}

template<typename... Bases>
std::optional<std::string> com_class_base<Bases...>
	::get_misc_status_thumbnail_raw() && noexcept
{
	return std::move(misc_status_thumbnail_);
}

template<typename... Bases>
const std::vector<std::string>& com_class_base<Bases...>
	::get_additional_progids() const& noexcept
{
	return additional_progids_;
}

template<typename... Bases>
std::vector<std::string>& com_class_base<Bases...>
	::get_additional_progids() & noexcept
{
	return additional_progids_;
}

template<typename... Bases>
std::vector<std::string> com_class_base<Bases...>
	::get_additional_progids() && noexcept
{
	return std::move(additional_progids_);
}

template<typename T1, typename T2, typename T3>
com_progid::com_progid(T1&& program, T2&& component, T3&& version)
	: program_(std::forward<T1>(program))
	, component_(std::forward<T2>(component))
	, version_(std::forward<T3>(version)) {
}

template<typename T1, typename T2>
com_progid::com_progid(T1&& program, T2&& component)
	: program_(std::forward<T1>(program))
	, component_(std::forward<T2>(component)) {
}

inline const std::string& com_progid::get_program() const& noexcept
{
	return program_;
}

inline std::string& com_progid::get_program() & noexcept
{
	return program_;
}

inline std::string com_progid::get_program() && noexcept
{
	return std::move(program_);
}

inline const std::string& com_progid::get_component() const& noexcept
{
	return component_;
}

inline std::string& com_progid::get_component() & noexcept
{
	return component_;
}

inline std::string com_progid::get_component() && noexcept
{
	return std::move(component_);
}

inline const std::optional<std::string>& com_progid::get_version() const& noexcept
{
	return version_;
}

inline std::optional<std::string>& com_progid::get_version() & noexcept
{
	return version_;
}

inline std::optional<std::string> com_progid::get_version() && noexcept
{
	return std::move(version_);
}

template<typename... Bases>
const std::string& com_typelib_base<Bases...>::get_tlbid_raw() const& noexcept
{
	return tlbid_;
}

template<typename... Bases>
std::string& com_typelib_base<Bases...>::get_tlbid_raw() & noexcept
{
	return tlbid_;
}

template<typename... Bases>
std::string com_typelib_base<Bases...>::get_tlbid_raw() && noexcept
{
	return std::move(tlbid_);
}

template<typename... Bases>
const std::string& com_typelib_base<Bases...>::get_version_raw() const& noexcept
{
	return version_;
}

template<typename... Bases>
std::string& com_typelib_base<Bases...>::get_version_raw() & noexcept
{
	return version_;
}

template<typename... Bases>
std::string com_typelib_base<Bases...>::get_version_raw() && noexcept
{
	return std::move(version_);
}

template<typename... Bases>
const std::string& com_typelib_base<Bases...>::get_help_dir() const& noexcept
{
	return help_dir_;
}

template<typename... Bases>
std::string& com_typelib_base<Bases...>::get_help_dir() & noexcept
{
	return help_dir_;
}

template<typename... Bases>
std::string com_typelib_base<Bases...>::get_help_dir() && noexcept
{
	return std::move(help_dir_);
}

template<typename... Bases>
const std::optional<std::string>& com_typelib_base<Bases...>
	::get_resource_id_raw() const& noexcept
{
	return resource_id_;
}

template<typename... Bases>
std::optional<std::string>& com_typelib_base<Bases...>
	::get_resource_id_raw() & noexcept
{
	return resource_id_;
}

template<typename... Bases>
std::optional<std::string> com_typelib_base<Bases...>
	::get_resource_id_raw() && noexcept
{
	return std::move(resource_id_);
}

template<typename... Bases>
const std::optional<std::string>& com_typelib_base<Bases...>
	::get_flags_raw() const& noexcept
{
	return flags_;
}

template<typename... Bases>
std::optional<std::string>& com_typelib_base<Bases...>
	::get_flags_raw() & noexcept
{
	return flags_;
}

template<typename... Bases>
std::optional<std::string> com_typelib_base<Bases...>
	::get_flags_raw() && noexcept
{
	return std::move(flags_);
}

template<typename... Bases>
const std::vector<std::string>& assembly_supported_os_list_base<Bases...>
	::get_list_raw() const& noexcept
{
	return supported_os_;
}

template<typename... Bases>
std::vector<std::string>& assembly_supported_os_list_base<Bases...>
	::get_list_raw() & noexcept
{
	return supported_os_;
}

template<typename... Bases>
std::vector<std::string> assembly_supported_os_list_base<Bases...>
	::get_list_raw() && noexcept
{
	return std::move(supported_os_);
}

template<typename... Bases>
const std::optional<std::string>& assembly_supported_os_list_base<Bases...>
	::get_max_tested_os_version_raw() const& noexcept
{
	return max_tested_os_version_;
}

template<typename... Bases>
std::optional<std::string>& assembly_supported_os_list_base<Bases...>
	::get_max_tested_os_version_raw() & noexcept
{
	return max_tested_os_version_;
}

template<typename... Bases>
std::optional<std::string> assembly_supported_os_list_base<Bases...>
	::get_max_tested_os_version_raw() && noexcept
{
	return std::move(max_tested_os_version_);
}

template<typename... Bases>
const std::string& com_interface_external_proxy_stub_base<Bases...>
	::get_iid_raw() const& noexcept
{
	return iid_;
}

template<typename... Bases>
std::string& com_interface_external_proxy_stub_base<Bases...>
	::get_iid_raw() & noexcept
{
	return iid_;
}

template<typename... Bases>
std::string com_interface_external_proxy_stub_base<Bases...>
	::get_iid_raw() && noexcept
{
	return std::move(iid_);
}

template<typename... Bases>
const std::optional<std::string>& com_interface_external_proxy_stub_base<Bases...>
	::get_base_interface_raw() const& noexcept
{
	return base_interface_;
}

template<typename... Bases>
std::optional<std::string>& com_interface_external_proxy_stub_base<Bases...>
	::get_base_interface_raw() & noexcept
{
	return base_interface_;
}

template<typename... Bases>
std::optional<std::string> com_interface_external_proxy_stub_base<Bases...>
	::get_base_interface_raw() && noexcept
{
	return std::move(base_interface_);
}

template<typename... Bases>
const std::optional<std::string>& com_interface_external_proxy_stub_base<Bases...>
	::get_name() const& noexcept
{
	return name_;
}

template<typename... Bases>
std::optional<std::string>& com_interface_external_proxy_stub_base<Bases...>
	::get_name() & noexcept
{
	return name_;
}

template<typename... Bases>
std::optional<std::string> com_interface_external_proxy_stub_base<Bases...>
	::get_name() && noexcept
{
	return std::move(name_);
}

template<typename... Bases>
const std::optional<std::string>& com_interface_external_proxy_stub_base<Bases...>
	::get_tlbid_raw() const& noexcept
{
	return tlbid_;
}

template<typename... Bases>
std::optional<std::string>& com_interface_external_proxy_stub_base<Bases...>
	::get_tlbid_raw() & noexcept
{
	return tlbid_;
}

template<typename... Bases>
std::optional<std::string> com_interface_external_proxy_stub_base<Bases...>
	::get_tlbid_raw() && noexcept
{
	return std::move(tlbid_);
}

template<typename... Bases>
const std::optional<std::string>& com_interface_external_proxy_stub_base<Bases...>
	::get_proxy_stub_clsid32_raw() const& noexcept
{
	return proxy_stub_clsid32_;
}

template<typename... Bases>
std::optional<std::string>& com_interface_external_proxy_stub_base<Bases...>
	::get_proxy_stub_clsid32_raw() & noexcept
{
	return proxy_stub_clsid32_;
}

template<typename... Bases>
std::optional<std::string> com_interface_external_proxy_stub_base<Bases...>
	::get_proxy_stub_clsid32_raw() && noexcept
{
	return std::move(proxy_stub_clsid32_);
}

template<typename... Bases>
const std::optional<std::string>& com_interface_external_proxy_stub_base<Bases...>
	::get_num_methods_raw() const& noexcept
{
	return num_methods_;
}

template<typename... Bases>
std::optional<std::string>& com_interface_external_proxy_stub_base<Bases...>
	::get_num_methods_raw() & noexcept
{
	return num_methods_;
}

template<typename... Bases>
std::optional<std::string> com_interface_external_proxy_stub_base<Bases...>
	::get_num_methods_raw() && noexcept
{
	return std::move(num_methods_);
}

template<typename... Bases>
const std::optional<std::string>& com_interface_proxy_stub_base<Bases...>
	::get_threading_model_raw() const& noexcept
{
	return threading_model_;
}

template<typename... Bases>
std::optional<std::string>& com_interface_proxy_stub_base<Bases...>
	::get_threading_model_raw() & noexcept
{
	return threading_model_;
}

template<typename... Bases>
std::optional<std::string> com_interface_proxy_stub_base<Bases...>
	::get_threading_model_raw() && noexcept
{
	return std::move(threading_model_);
}

template<typename... Bases>
const std::string& window_class_base<Bases...>::get_class() const& noexcept
{
	return class_;
}

template<typename... Bases>
std::string& window_class_base<Bases...>::get_class() & noexcept
{
	return class_;
}

template<typename... Bases>
std::string window_class_base<Bases...>::get_class() && noexcept
{
	return std::move(class_);
}

template<typename... Bases>
const std::optional<std::string>& window_class_base<Bases...>
	::is_versioned_raw() const& noexcept
{
	return versioned_;
}

template<typename... Bases>
std::optional<std::string>& window_class_base<Bases...>
	::is_versioned_raw() & noexcept
{
	return versioned_;
}

template<typename... Bases>
std::optional<std::string> window_class_base<Bases...>
	::is_versioned_raw() && noexcept
{
	return std::move(versioned_);
}


template<typename... Bases>
const std::string& native_manifest_base<Bases...>
	::get_manifest_version() const& noexcept
{
	return manifest_version_;
}

template<typename... Bases>
std::string& native_manifest_base<Bases...>
	::get_manifest_version() & noexcept
{
	return manifest_version_;
}

template<typename... Bases>
std::string native_manifest_base<Bases...>
	::get_manifest_version() && noexcept
{
	return std::move(manifest_version_);
}

template<typename... Bases>
assembly_no_inherit native_manifest_base<Bases...>
	::no_inherit() const noexcept
{
	return no_inherit_;
}

template<typename... Bases>
const assembly_identity_base<Bases...>& native_manifest_base<Bases...>
	::get_assembly_identity() const& noexcept
{
	return assembly_identity_;
}

template<typename... Bases>
assembly_identity_base<Bases...>& native_manifest_base<Bases...>
	::get_assembly_identity() & noexcept
{
	return assembly_identity_;
}

template<typename... Bases>
assembly_identity_base<Bases...> native_manifest_base<Bases...>
	::get_assembly_identity() && noexcept
{
	return std::move(assembly_identity_);
}

template<typename... Bases>
const std::optional<assembly_supported_os_list_base<Bases...>>&
	native_manifest_base<Bases...>::get_supported_os_list() const& noexcept
{
	return supported_os_list_;
}

template<typename... Bases>
std::optional<assembly_supported_os_list_base<Bases...>>&
	native_manifest_base<Bases...>::get_supported_os_list() & noexcept
{
	return supported_os_list_;
}

template<typename... Bases>
std::optional<assembly_supported_os_list_base<Bases...>>
	native_manifest_base<Bases...>::get_supported_os_list() && noexcept
{
	return std::move(supported_os_list_);
}

template<typename... Bases>
const std::vector<assembly_identity_base<Bases...>>&
	native_manifest_base<Bases...>::get_dependencies() const& noexcept
{
	return dependencies_;
}

template<typename... Bases>
std::vector<assembly_identity_base<Bases...>>&
	native_manifest_base<Bases...>::get_dependencies() & noexcept
{
	return dependencies_;
}

template<typename... Bases>
std::vector<assembly_identity_base<Bases...>>
	native_manifest_base<Bases...>::get_dependencies() && noexcept
{
	return std::move(dependencies_);
}

template<typename... Bases>
const std::vector<typename native_manifest_base<Bases...>
	::assembly_file_type>& native_manifest_base<Bases...>
	::get_files() const& noexcept
{
	return files_;
}

template<typename... Bases>
std::vector<typename native_manifest_base<Bases...>
	::assembly_file_type>& native_manifest_base<Bases...>
	::get_files() & noexcept
{
	return files_;
}

template<typename... Bases>
std::vector<typename native_manifest_base<Bases...>
	::assembly_file_type> native_manifest_base<Bases...>
	::get_files() && noexcept
{
	return std::move(files_);
}

template<typename... Bases>
const std::optional<active_code_page>& native_manifest_base<Bases...>
	::get_active_code_page() const& noexcept
{
	return active_code_page_;
}

template<typename... Bases>
std::optional<active_code_page>& native_manifest_base<Bases...>
	::get_active_code_page() & noexcept
{
	return active_code_page_;
}

template<typename... Bases>
std::optional<active_code_page> native_manifest_base<Bases...>
	::get_active_code_page() && noexcept
{
	return std::move(active_code_page_);
}

template<typename... Bases>
const std::optional<std::string>& native_manifest_base<Bases...>
	::get_description() const& noexcept
{
	return description_;
}

template<typename... Bases>
std::optional<std::string>& native_manifest_base<Bases...>
	::get_description() & noexcept
{
	return description_;
}

template<typename... Bases>
std::optional<std::string> native_manifest_base<Bases...>
	::get_description() && noexcept
{
	return std::move(description_);
}

template<typename... Bases>
bool native_manifest_base<Bases...>::auto_elevate() const noexcept
{
	return auto_elevate_;
}

template<typename... Bases>
bool native_manifest_base<Bases...>::disable_theming() const noexcept
{
	return disable_theming_;
}

template<typename... Bases>
bool native_manifest_base<Bases...>::disable_window_filtering() const noexcept
{
	return disable_window_filtering_;
}

template<typename... Bases>
const dpi_awareness& native_manifest_base<Bases...>::get_dpi_awareness() const& noexcept
{
	return dpi_awareness_;
}

template<typename... Bases>
dpi_awareness& native_manifest_base<Bases...>::get_dpi_awareness() & noexcept
{
	return dpi_awareness_;
}

template<typename... Bases>
dpi_awareness native_manifest_base<Bases...>::get_dpi_awareness() && noexcept
{
	return std::move(dpi_awareness_);
}

template<typename... Bases>
bool native_manifest_base<Bases...>::gdi_scaling() const noexcept
{
	return gdi_scaling_;
}

template<typename... Bases>
bool native_manifest_base<Bases...>::high_resolution_scrolling_aware() const noexcept
{
	return high_resolution_scrolling_aware_;
}

template<typename... Bases>
bool native_manifest_base<Bases...>::long_path_aware() const noexcept
{
	return long_path_aware_;
}

template<typename... Bases>
bool native_manifest_base<Bases...>::printer_driver_isolation() const noexcept
{
	return printer_driver_isolation_;
}

template<typename... Bases>
bool native_manifest_base<Bases...>::ultra_high_resolution_scrolling_aware() const noexcept
{
	return ultra_high_resolution_scrolling_aware_;
}

template<typename... Bases>
void native_manifest_base<Bases...>
	::set_auto_elevate(bool value) noexcept
{
	auto_elevate_ = value;
}

template<typename... Bases>
void native_manifest_base<Bases...>
	::set_disable_theming(bool value) noexcept
{
	disable_theming_ = value;
}

template<typename... Bases>
void native_manifest_base<Bases...>
	::set_disable_window_filtering(bool value) noexcept
{
	disable_window_filtering_ = value;
}

template<typename... Bases>
void native_manifest_base<Bases...>
	::set_gdi_scaling(bool value) noexcept
{
	gdi_scaling_ = value;
}

template<typename... Bases>
void native_manifest_base<Bases...>
	::set_high_resolution_scrolling_aware(bool value) noexcept
{
	high_resolution_scrolling_aware_ = value;
}

template<typename... Bases>
void native_manifest_base<Bases...>
	::set_long_path_aware(bool value) noexcept
{
	long_path_aware_ = value;
}

template<typename... Bases>
void native_manifest_base<Bases...>
	::set_printer_driver_isolation(bool value) noexcept
{
	printer_driver_isolation_ = value;
}

template<typename... Bases>
void native_manifest_base<Bases...>
	::set_ultra_high_resolution_scrolling_aware(bool value) noexcept
{
	ultra_high_resolution_scrolling_aware_ = value;
}

template<typename... Bases>
const std::optional<heap_type>& native_manifest_base<Bases...>
	::get_heap_type() const& noexcept
{
	return heap_type_;
}

template<typename... Bases>
std::optional<heap_type>& native_manifest_base<Bases...>
	::get_heap_type() & noexcept
{
	return heap_type_;
}

template<typename... Bases>
std::optional<heap_type> native_manifest_base<Bases...>
	::get_heap_type() && noexcept
{
	return heap_type_;
}

template<typename... Bases>
const std::optional<requested_privileges>&
	native_manifest_base<Bases...>::get_requested_privileges() const& noexcept
{
	return requested_privileges_;
}

template<typename... Bases>
std::optional<requested_privileges>&
	native_manifest_base<Bases...>::get_requested_privileges() & noexcept
{
	return requested_privileges_;
}

template<typename... Bases>
std::optional<requested_privileges>
	native_manifest_base<Bases...>::get_requested_privileges() && noexcept
{
	return std::move(requested_privileges_);
}

template<typename... Bases>
const std::vector<typename native_manifest_base<Bases...>
	::com_interface_external_proxy_stub_type>& native_manifest_base<Bases...>
	::get_com_interface_external_proxy_stubs() const& noexcept
{
	return com_interface_external_proxy_stubs_;
}

template<typename... Bases>
std::vector<typename native_manifest_base<Bases...>
	::com_interface_external_proxy_stub_type>& native_manifest_base<Bases...>
	::get_com_interface_external_proxy_stubs() & noexcept
{
	return com_interface_external_proxy_stubs_;
}

template<typename... Bases>
std::vector<typename native_manifest_base<Bases...>
	::com_interface_external_proxy_stub_type> native_manifest_base<Bases...>
	::get_com_interface_external_proxy_stubs() && noexcept
{
	return std::move(com_interface_external_proxy_stubs_);
}

template<typename... Bases>
const std::optional<typename native_manifest_base<Bases...>
	::msix_identity_type>& native_manifest_base<Bases...>
	::get_msix_identity() const& noexcept
{
	return msix_identity_;
}

template<typename... Bases>
std::optional<typename native_manifest_base<Bases...>
	::msix_identity_type>& native_manifest_base<Bases...>
	::get_msix_identity() & noexcept
{
	return msix_identity_;
}

template<typename... Bases>
std::optional<typename native_manifest_base<Bases...>
	::msix_identity_type>native_manifest_base<Bases...>
	::get_msix_identity() && noexcept
{
	return std::move(msix_identity_);
}

template<typename... Bases>
void native_manifest_base<Bases...>
	::set_no_inherit(assembly_no_inherit value) noexcept
{
	no_inherit_ = value;
}

template<typename... Bases>
const std::string& msix_identity_base<Bases...>::get_publisher() const& noexcept
{
	return publisher_;
}

template<typename... Bases>
std::string& msix_identity_base<Bases...>::get_publisher() & noexcept
{
	return publisher_;
}

template<typename... Bases>
std::string msix_identity_base<Bases...>::get_publisher() && noexcept
{
	return std::move(publisher_);
}

template<typename... Bases>
const std::string& msix_identity_base<Bases...>::get_package_name() const& noexcept
{
	return package_name_;
}

template<typename... Bases>
std::string& msix_identity_base<Bases...>::get_package_name() & noexcept
{
	return package_name_;
}

template<typename... Bases>
std::string msix_identity_base<Bases...>::get_package_name() && noexcept
{
	return std::move(package_name_);
}

template<typename... Bases>
const std::string& msix_identity_base<Bases...>::get_application_id() const& noexcept
{
	return application_id_;
}

template<typename... Bases>
std::string& msix_identity_base<Bases...>::get_application_id() & noexcept
{
	return application_id_;
}

template<typename... Bases>
std::string msix_identity_base<Bases...>::get_application_id() && noexcept
{
	return std::move(application_id_);
}

} //namespace pe_bliss::resources
