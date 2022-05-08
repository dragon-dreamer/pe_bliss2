#pragma once

#include <cstdint>

#include "pe_bliss2/pe_error.h"

namespace pe_bliss::core
{

class optional_header;

struct [[nodiscard]] optional_header_validation_options
{
	bool validate_address_of_entry_point = true;
	bool validate_alignments = true;
	bool validate_subsystem_version = true;
	bool validate_size_of_heap = true;
	bool validate_size_of_stack = true;
	bool validate_size_of_headers = true;
};

// validate() method does not validate image base, there is
// a separate validate_image_base() method which should be used.
// validate() method does not validate size of optional header, there is
// a separate validate_size_of_optional_header() method which should be used.
[[nodiscard]]
pe_error_wrapper validate(const optional_header& header,
	const optional_header_validation_options& options,
	bool is_dll) noexcept;
[[nodiscard]]
pe_error_wrapper validate_address_of_entry_point(
	const optional_header& header, bool is_dll) noexcept;
[[nodiscard]]
pe_error_wrapper validate_image_base(
	const optional_header& header, bool has_relocations) noexcept;
[[nodiscard]]
pe_error_wrapper validate_file_alignment(const optional_header& header) noexcept;
[[nodiscard]]
pe_error_wrapper validate_section_alignment(const optional_header& header) noexcept;
[[nodiscard]]
pe_error_wrapper validate_subsystem_version(const optional_header& header) noexcept;
[[nodiscard]]
pe_error_wrapper validate_size_of_heap(const optional_header& header) noexcept;
[[nodiscard]]
pe_error_wrapper validate_size_of_stack(const optional_header& header) noexcept;
[[nodiscard]]
pe_error_wrapper validate_size_of_headers(const optional_header& header) noexcept;
[[nodiscard]]
pe_error_wrapper validate_size_of_optional_header(
	std::uint16_t size_of_optional_header, const optional_header& hdr) noexcept;
} //namespace pe_bliss::core
