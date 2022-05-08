#pragma once

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/error_list.h"

namespace pe_bliss
{
class error_list;
} //namespace pe_bliss

namespace pe_bliss::dos
{

class dos_header;

struct dos_header_validation_options
{
	bool validate_e_lfanew = true;
	bool validate_magic = true;
};

[[nodiscard]]
bool validate(const dos_header& header,
	const dos_header_validation_options& options,
	error_list& errors) noexcept;
[[nodiscard]]
pe_error_wrapper validate_magic(const dos_header& header) noexcept;
[[nodiscard]]
pe_error_wrapper validate_e_lfanew(const dos_header& header) noexcept;

} //namespace pe_bliss::dos
