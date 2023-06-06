#pragma once

#include "pe_bliss2/pe_error.h"

namespace pe_bliss::core
{

class image_signature;

[[nodiscard]]
pe_error_wrapper validate(const image_signature& header) noexcept;

} //namespace pe_bliss::core
