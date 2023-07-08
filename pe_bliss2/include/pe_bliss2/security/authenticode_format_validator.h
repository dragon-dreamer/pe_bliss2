#pragma once

#include <system_error>
#include <type_traits>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/authenticode_pkcs7.h"

namespace pe_bliss::security
{

enum class authenticode_format_validator_errc
{
	invalid_signed_data_oid,
	invalid_signed_data_version,
	invalid_signer_count,
	non_matching_digest_algorithm,
	invalid_content_info_oid,
	invalid_signer_info_version,
	absent_message_digest,
	invalid_message_digest,
	absent_content_type,
	invalid_content_type,
	invalid_type_value_type,
	non_matching_type_value_digest_algorithm
};

std::error_code make_error_code(authenticode_format_validator_errc) noexcept;

template<typename RangeType>
void validate(const authenticode_pkcs7<RangeType>& signature,
	const authenticode_attribute_map<RangeType> authenticated_attributes,
	error_list& errors);

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::authenticode_format_validator_errc> : true_type {};
} //namespace std
