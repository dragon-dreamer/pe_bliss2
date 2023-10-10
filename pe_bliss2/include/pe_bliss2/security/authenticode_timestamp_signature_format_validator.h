#pragma once

#include <system_error>
#include <type_traits>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/authenticode_timestamp_signature.h"

#include "simple_asn1/types.h"

namespace pe_bliss::security
{

enum class authenticode_timestamp_signature_format_validator_errc
{
	invalid_tst_info_version = 1,
	invalid_tst_info_accuracy_value
};

std::error_code make_error_code(
	authenticode_timestamp_signature_format_validator_errc) noexcept;

template<typename RangeType>
void validate_autenticode_timestamp_format(
	const authenticode_signature_cms_info_ms_bug_workaround_type<RangeType>& signature,
	error_list& errors);
template<typename RangeType>
void validate_autenticode_timestamp_format(
	const authenticode_signature_cms_info_type<RangeType>& signature,
	error_list& errors);

template<typename RangeType>
void validate_autenticode_timestamp_format(
	const pkcs7::attribute_map<RangeType>& authenticated_attributes,
	error_list& errors);

template<typename RangeType>
void validate_autenticode_timestamp_format(
	const authenticode_timestamp_signature<RangeType>& signature,
	error_list& errors);

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::authenticode_timestamp_signature_format_validator_errc>
	: true_type {};
} //namespace std
