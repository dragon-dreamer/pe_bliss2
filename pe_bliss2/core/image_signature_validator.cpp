#include "pe_bliss2/core/image_signature_validator.h"

#include "pe_bliss2/core/image_signature.h"
#include "pe_bliss2/core/image_signature_errc.h"

namespace pe_bliss::core
{

pe_error_wrapper validate(const image_signature& header) noexcept
{
	if (header.get_descriptor().get() != image_signature::pe_signature)
		return image_signature_errc::invalid_pe_signature;
	return {};
}

} //namespace pe_bliss::core
