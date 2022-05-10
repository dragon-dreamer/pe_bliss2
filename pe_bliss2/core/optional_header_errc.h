#pragma once

#include <system_error>
#include <type_traits>

namespace pe_bliss::core
{

enum class optional_header_errc
{
	invalid_pe_magic = 1,
	invalid_address_of_entry_point,
	unaligned_image_base,
	too_large_image_base,
	incorrect_section_alignment,
	incorrect_file_alignment,
	file_alignment_out_of_range,
	section_alignment_out_of_range,
	too_low_subsystem_version,
	invalid_size_of_heap,
	invalid_size_of_stack,
	invalid_size_of_headers,
	no_base_of_data_field,
	unable_to_read_optional_header,
	invalid_size_of_optional_header,
	invalid_size_of_image
};

std::error_code make_error_code(optional_header_errc) noexcept;

} //namespace pe_bliss::detail::core

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::core::optional_header_errc> : true_type {};
} //namespace std
