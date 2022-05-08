#include "pe_bliss2/dos/dos_header_validator.h"

#include "pe_bliss2/dos/dos_header.h"
#include "pe_bliss2/dos/dos_header_errc.h"

#include "utilities/math.h"

namespace pe_bliss::dos
{

pe_error_wrapper validate(const dos_header& header,
	const dos_header_validation_options& options) noexcept
{
	pe_error_wrapper result;
	if (options.validate_magic && (result = validate_magic(header)))
		return result;

	if (options.validate_e_lfanew && (result = validate_e_lfanew(header)))
		return result;

	return result;
}

pe_error_wrapper validate_magic(const dos_header& header) noexcept
{
	if (header.base_struct()->e_magic != dos_header::mz_magic_value)
		return dos_header_errc::invalid_dos_header_signature;
	return {};
}

pe_error_wrapper validate_e_lfanew(const dos_header& header) noexcept
{
	auto e_lfanew = header.base_struct()->e_lfanew;

	if (!utilities::math::is_aligned<std::uint32_t>(e_lfanew))
		return dos_header_errc::unaligned_e_lfanew;

	if (e_lfanew < dos_header::min_e_lfanew
		|| e_lfanew > dos_header::max_e_lfanew)
	{
		return dos_header_errc::invalid_e_lfanew;
	}

	return {};
}


} //namespace pe_bliss::dos
