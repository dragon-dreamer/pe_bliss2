#pragma once

#include <system_error>
#include <type_traits>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/authenticode_pkcs7.h"

#include "simple_asn1/types.h"

namespace pe_bliss::security
{

enum class authenticode_format_validator_errc
{
	invalid_content_info_oid,
	invalid_type_value_type,
	non_matching_type_value_digest_algorithm
};

std::error_code make_error_code(authenticode_format_validator_errc) noexcept;

template<typename RangeType>
void validate(const authenticode_pkcs7<RangeType>& signature,
	error_list& errors);

template<typename RangeType>
void validate(const pkcs7::attribute_map<RangeType>& authenticated_attributes,
	std::optional<asn1::utc_time>& signing_time, error_list& errors);

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::authenticode_format_validator_errc> : true_type {};
} //namespace std
