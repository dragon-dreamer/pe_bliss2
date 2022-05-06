#pragma once

#include <system_error>
#include <type_traits>

namespace pe_bliss::section
{

enum class section_errc
{
	invalid_section_raw_size = 1,
	invalid_section_virtual_size,
	invalid_section_raw_address,
	invalid_section_raw_size_alignment,
	invalid_section_raw_address_alignment,
	raw_section_size_overflow,
	virtual_section_size_overflow,
	invalid_section_virtual_address_alignment,
	virtual_gap_between_sections,
	invalid_section_low_alignment,
	virtual_gap_between_headers_and_first_section,
	invalid_size_of_image,
	unable_to_read_section_table,
	invalid_section_offset,
	unable_to_read_section_data
};

std::error_code make_error_code(section_errc) noexcept;

} //namespace pe_bliss::section

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::section::section_errc> : true_type {};
} //namespace std
