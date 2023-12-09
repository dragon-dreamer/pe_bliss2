#pragma once

#include <exception>
#include <optional>

#include "pe_bliss2/security/authenticode_check_status.h"
#include "pe_bliss2/security/authenticode_verification_options.h"
#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/security_directory.h"

namespace pe_bliss::image { class image; }

namespace pe_bliss::security
{

struct [[nodiscard]] image_authenticode_check_status
{
	authenticode_check_status<span_range_type> authenticode_status;
	std::exception_ptr error;
};

[[nodiscard]]
std::optional<image_authenticode_check_status> verify_authenticode(
	const image::image& instance,
	const authenticode_verification_options& opts = {});

template<typename... Bases>
[[nodiscard]]
std::optional<image_authenticode_check_status> verify_authenticode(
	const image::image& instance,
	const security_directory_base<Bases...>& sec_dir,
	const authenticode_verification_options& opts = {});

} //namespace pe_bliss::security
