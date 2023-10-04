#pragma once

#include <system_error>
#include <type_traits>

namespace pe_bliss::security
{

enum class authenticode_format_validator_errc
{
	invalid_content_info_oid = 1,
	invalid_type_value_type,
	non_matching_type_value_digest_algorithm
};

std::error_code make_error_code(authenticode_format_validator_errc) noexcept;

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::authenticode_format_validator_errc> : true_type {};
} //namespace std
