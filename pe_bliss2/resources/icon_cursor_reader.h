#pragma once

#include <cstddef>
#include <string_view>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/resources/icon_cursor.h"
#include "pe_bliss2/resources/resource_directory.h"
#include "pe_bliss2/resources/resource_types.h"

namespace pe_bliss::resources
{

enum class icon_cursor_reader_errc
{
	unable_to_read_header = 1,
	unable_to_read_group_entry_header,
	unable_to_read_data
};

std::error_code make_error_code(icon_cursor_reader_errc) noexcept;

struct [[nodiscard]] icon_cursor_read_options
{
	bool allow_virtual_data = false;
	bool copy_memory = false;
};

template<typename... Bases>
[[nodiscard]] icon_group icon_group_from_resource(
	const resource_directory_base<Bases...>& root,
	std::size_t language_index, resource_id_type icon_group_id,
	const icon_cursor_read_options& options = {});
template<typename... Bases>
[[nodiscard]] icon_group icon_group_from_resource(
	const resource_directory_base<Bases...>& root,
	std::size_t language_index, std::u16string_view icon_group_name,
	const icon_cursor_read_options& options = {});

template<typename... Bases>
[[nodiscard]] icon_group icon_group_from_resource_by_lang(
	const resource_directory_base<Bases...>& root,
	resource_id_type language, resource_id_type icon_group_id,
	const icon_cursor_read_options& options = {});
template<typename... Bases>
[[nodiscard]] icon_group icon_group_from_resource_by_lang(
	const resource_directory_base<Bases...>& root,
	resource_id_type language, std::u16string_view icon_group_name,
	const icon_cursor_read_options& options = {});

template<typename... Bases>
[[nodiscard]] cursor_group cursor_group_from_resource(
	const resource_directory_base<Bases...>& root,
	std::size_t language_index, resource_id_type cursor_group_id,
	const icon_cursor_read_options& options = {});
template<typename... Bases>
[[nodiscard]] cursor_group cursor_group_from_resource(
	const resource_directory_base<Bases...>& root,
	std::size_t language_index, std::u16string_view cursor_group_name,
	const icon_cursor_read_options& options = {});

template<typename... Bases>
[[nodiscard]] cursor_group cursor_group_from_resource_by_lang(
	const resource_directory_base<Bases...>& root,
	resource_id_type language, resource_id_type cursor_group_id,
	const icon_cursor_read_options& options = {});
template<typename... Bases>
[[nodiscard]] cursor_group cursor_group_from_resource_by_lang(
	const resource_directory_base<Bases...>& root,
	resource_id_type language, std::u16string_view cursor_group_name,
	const icon_cursor_read_options& options = {});

} //namespace pe_bliss::resources

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::resources::icon_cursor_reader_errc> : true_type {};
} //namespace std
