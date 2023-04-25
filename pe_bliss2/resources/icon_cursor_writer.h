#pragma once

#include <system_error>
#include <type_traits>

#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/resources/icon_cursor.h"

namespace buffers
{
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::resources
{

enum class icon_cursor_writer_errc
{
	different_number_of_headers_and_data = 1,
	invalid_hotspot
};

std::error_code make_error_code(icon_cursor_writer_errc) noexcept;

class [[nodiscard]] icon_dir_entry_header final
	: public icon_group_header_base<detail::resources::icondirentry>
{
};

class [[nodiscard]] cursor_dir_entry_header final
	: public icon_group_header_base<detail::resources::cursordirentry>
{
};

using file_icon_group = icon_cursor_group<detail::resources::ico_header,
	icon_dir_entry_header, detail::resources::icon_type>;
using file_cursor_group = icon_cursor_group<detail::resources::cursor_header,
	cursor_dir_entry_header, detail::resources::cursor_type>;

struct [[nodiscard]] icon_cursor_write_options
{
	bool write_virtual_part = false;
};

[[nodiscard]]
file_icon_group to_file_format(const icon_group& group,
	const icon_cursor_write_options& options = {});
[[nodiscard]]
file_icon_group to_file_format(icon_group&& group,
	const icon_cursor_write_options& options = {});

[[nodiscard]]
file_cursor_group to_file_format(const cursor_group& group,
	const icon_cursor_write_options& options = {});
[[nodiscard]]
file_cursor_group to_file_format(cursor_group&& group,
	const icon_cursor_write_options& options = {});

void write_icon(const file_icon_group& group,
	buffers::output_buffer_interface& buf,
	const icon_cursor_write_options& options = {});
void write_cursor(const file_cursor_group& group,
	buffers::output_buffer_interface& buf,
	const icon_cursor_write_options& options = {});

} //namespace pe_bliss::resources

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::resources::icon_cursor_writer_errc> : true_type {};
} //namespace std
