#include "pe_bliss2/section/section_header_validator.h"

#include <bit>

#include "pe_bliss2/section/section_header.h"
#include "pe_bliss2/section/section_errc.h"

#include "utilities/math.h"

namespace pe_bliss::section
{

pe_error_wrapper validate_raw_size(const section_header& header,
	std::uint32_t section_alignment) noexcept
{
	return header.get_raw_size(section_alignment)
		< section_header::two_gb_size
		? pe_error_wrapper{} : section_errc::invalid_section_raw_size;
}

pe_error_wrapper validate_virtual_size(
	const section_header& header) noexcept
{
	const auto virtual_size = header.get_descriptor()->virtual_size;
	if (!virtual_size)
	{
		if (!header.get_descriptor()->size_of_raw_data)
			return section_errc::invalid_section_virtual_size;
	}
	return virtual_size < section_header::two_gb_size
		? pe_error_wrapper{} : section_errc::invalid_section_virtual_size;
}

pe_error_wrapper validate_raw_address(
	const section_header& header) noexcept
{
	const auto& base_struct = header.get_descriptor().get();
	if (base_struct.size_of_raw_data == 0
		&& base_struct.virtual_size == 0)
	{
		return section_errc::invalid_section_raw_address;
	}

	if (base_struct.size_of_raw_data != 0
		&& base_struct.pointer_to_raw_data == 0)
	{
		return section_errc::invalid_section_raw_address;
	}

	return {};
}

pe_error_wrapper validate_raw_size_alignment(
	const section_header& header,
	std::uint32_t section_alignment, std::uint32_t file_alignment,
	bool last_section) noexcept
{
	if (!std::has_single_bit(section_alignment)
		|| !std::has_single_bit(file_alignment))
	{
		return section_errc::invalid_section_raw_size_alignment;
	}
	return last_section
		|| (header.get_raw_size(section_alignment) % file_alignment) == 0
		? pe_error_wrapper{} : section_errc::invalid_section_raw_size_alignment;
}

pe_error_wrapper validate_raw_address_alignment(
	const section_header& header,
	std::uint32_t file_alignment) noexcept
{
	if (!std::has_single_bit(file_alignment))
		return section_errc::invalid_section_raw_address_alignment;
	return (header.get_pointer_to_raw_data() % file_alignment) == 0
		? pe_error_wrapper{} : section_errc::invalid_section_raw_address_alignment;
}

pe_error_wrapper validate_virtual_address_alignment(
	const section_header& header,
	std::uint32_t section_alignment) noexcept
{
	if (!std::has_single_bit(section_alignment))
		return section_errc::invalid_section_virtual_address_alignment;
	return (header.get_descriptor()->virtual_address % section_alignment) == 0
		? pe_error_wrapper{} : section_errc::invalid_section_virtual_address_alignment;
}

pe_error_wrapper validate_raw_size_bounds(
	const section_header& header,
	std::uint32_t section_alignment) noexcept
{
	return utilities::math::is_sum_safe(
		header.get_raw_size(section_alignment),
		header.get_pointer_to_raw_data())
		? pe_error_wrapper{} : section_errc::raw_section_size_overflow;
}

pe_error_wrapper validate_virtual_size_bounds(
	const section_header& header,
	std::uint32_t section_alignment) noexcept
{
	return utilities::math::is_sum_safe(
		header.get_virtual_size(section_alignment), header.get_rva())
		? pe_error_wrapper{} : section_errc::virtual_section_size_overflow;
}

} //namespace pe_bliss::section
