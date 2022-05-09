#pragma once

#include <cstdint>

#include "pe_bliss2/pe_error.h"

namespace pe_bliss::section
{

class section_header;

[[nodiscard]]
pe_error_wrapper validate_raw_size(const section_header& header,
	std::uint32_t section_alignment) noexcept;
[[nodiscard]]
pe_error_wrapper validate_virtual_size(
	const section_header& header) noexcept;
[[nodiscard]]
pe_error_wrapper validate_raw_address(
	const section_header& header) noexcept;
[[nodiscard]]
pe_error_wrapper validate_raw_size_alignment(
	const section_header& header,
	std::uint32_t section_alignment, std::uint32_t file_alignment,
	bool last_section) noexcept;
[[nodiscard]]
pe_error_wrapper validate_raw_address_alignment(
	const section_header& header,
	std::uint32_t file_alignment) noexcept;
[[nodiscard]]
pe_error_wrapper validate_virtual_address_alignment(
	const section_header& header,
	std::uint32_t section_alignment) noexcept;
[[nodiscard]]
pe_error_wrapper validate_raw_size_bounds(
	const section_header& header,
	std::uint32_t section_alignment) noexcept;
[[nodiscard]]
pe_error_wrapper validate_virtual_size_bounds(
	const section_header& header,
	std::uint32_t section_alignment) noexcept;

} //namespace pe_bliss::section
