#include "pe_bliss2/dos/dos_header_validator.h"

#include "pe_bliss2/dos/dos_header.h"
#include "pe_bliss2/dos/dos_header_errc.h"
#include "pe_bliss2/error_list.h"

#include "utilities/math.h"

namespace pe_bliss::dos
{

bool validate(const dos_header& header,
	const dos_header_validation_options& options,
	error_list& errors) noexcept
{
	pe_error_wrapper err;
	if (options.validate_magic && (err = validate_magic(header)))
		errors.add_error(err);

	if (options.validate_e_lfanew && (err = validate_e_lfanew(header)))
		errors.add_error(err);

	return !errors.has_errors();
}

pe_error_wrapper validate_magic(const dos_header& header) noexcept
{
	if (header.get_descriptor()->e_magic != dos_header::mz_magic_value)
		return dos_header_errc::invalid_dos_header_signature;
	return {};
}

pe_error_wrapper validate_e_lfanew(const dos_header& header) noexcept
{
	auto e_lfanew = header.get_descriptor()->e_lfanew;

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
