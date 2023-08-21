#include "pe_bliss2/security/authenticode_timestamp_signature_format_validator.h"

#include <algorithm>
#include <optional>

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/authenticode_format_validator_errc.h"
#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/pkcs7/pkcs7_format_validator.h"

#include "simple_asn1/crypto/crypto_common_types.h"
#include "simple_asn1/crypto/pkcs7/types.h"
#include "simple_asn1/crypto/pkcs9/oids.h"

namespace pe_bliss::security
{

namespace
{

template<typename T>
void validate_autenticode_timestamp_format_impl(
	const T& signature, error_list& errors)
{
	static constexpr std::int32_t cms_info_version = 3u;
	pkcs7::validate(signature, errors, cms_info_version);

	const auto& content_info = signature.get_content_info();
	if (content_info.data.digest_algorithms.size() != 1u
		|| content_info.data.signer_infos.size() != 1u)
	{
		errors.add_error(pkcs7::pkcs7_format_validator_errc::invalid_signer_count);
		return;
	}

	const auto& signed_data_content_info = content_info.data.content_info;
	if (!std::ranges::equal(signed_data_content_info.content_type.container,
		asn1::crypto::pkcs9::oid_tst_info))
	{
		errors.add_error(authenticode_format_validator_errc::invalid_content_info_oid);
	}

	//TODO: validate tst_info fields better
}

} //namespace

template<typename RangeType>
void validate_autenticode_timestamp_format(
	const authenticode_signature_cms_info_type<RangeType>& signature,
	error_list& errors)
{
	validate_autenticode_timestamp_format_impl(signature, errors);
}

template<typename RangeType>
void validate_autenticode_timestamp_format(
	const authenticode_signature_cms_info_ms_bug_workaround_type<RangeType>& signature,
	error_list& errors)
{
	validate_autenticode_timestamp_format_impl(signature, errors);
}

template<typename RangeType>
void validate_autenticode_timestamp_format(
	const pkcs7::attribute_map<RangeType>& authenticated_attributes,
	error_list& errors)
{
	std::optional<asn1::crypto::object_identifier_type> content_type;
	std::optional<asn1::utc_time> signing_time;
	pkcs7::validate_authenticated_attributes(authenticated_attributes,
		signing_time, content_type, errors);

	if (content_type && !std::ranges::equal(content_type->container,
		asn1::crypto::pkcs9::oid_tst_info))
	{
		errors.add_error(pkcs7::pkcs7_format_validator_errc::invalid_content_type);
	}
}

template void validate_autenticode_timestamp_format<span_range_type>(
	const authenticode_signature_cms_info_ms_bug_workaround_type<span_range_type>& signature,
	error_list& errors);
template void validate_autenticode_timestamp_format<vector_range_type>(
	const authenticode_signature_cms_info_ms_bug_workaround_type<vector_range_type>& signature,
	error_list& errors);
template void validate_autenticode_timestamp_format<span_range_type>(
	const authenticode_signature_cms_info_type<span_range_type>& signature,
	error_list& errors);
template void validate_autenticode_timestamp_format<vector_range_type>(
	const authenticode_signature_cms_info_type<vector_range_type>& signature,
	error_list& errors);

template void validate_autenticode_timestamp_format<span_range_type>(
	const pkcs7::attribute_map<span_range_type>& authenticated_attributes,
	error_list& errors);
template void validate_autenticode_timestamp_format<vector_range_type>(
	const pkcs7::attribute_map<vector_range_type>& authenticated_attributes,
	error_list& errors);

} //namespace pe_bliss::security
