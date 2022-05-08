#pragma once

#include "pe_bliss2/pe_error.h"

namespace pe_bliss::dos
{

class dos_header;

struct dos_header_validation_options
{
	bool validate_e_lfanew = true;
	bool validate_magic = true;
};

[[nodiscard]]
pe_error_wrapper validate(const dos_header& header,
	const dos_header_validation_options& options) noexcept;
[[nodiscard]]
pe_error_wrapper validate_magic(const dos_header& header) noexcept;
[[nodiscard]]
pe_error_wrapper validate_e_lfanew(const dos_header& header) noexcept;

} //namespace pe_bliss::dos
