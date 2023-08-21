#include "pe_bliss2/security/authenticode_format_validator.h"

#include <algorithm>

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/authenticode_format_validator_errc.h"
#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/crypto_algorithms.h"
#include "pe_bliss2/security/pkcs7/pkcs7_format_validator.h"

#include "simple_asn1/crypto/crypto_common_types.h"
#include "simple_asn1/crypto/pkcs7/authenticode/oids.h"

namespace pe_bliss::security
{

template<typename RangeType>
void validate_autenticode_format(const authenticode_pkcs7<RangeType>& signature,
	error_list& errors)
{
	pkcs7::validate(signature, errors);

	const auto& content_info = signature.get_content_info();
	if (content_info.data.digest_algorithms.size() != 1u
		|| content_info.data.signer_infos.size() != 1u)
	{
		errors.add_error(pkcs7::pkcs7_format_validator_errc::invalid_signer_count);
		return;
	}

	const auto& signed_data_content_info = content_info.data.content_info;
	if (!std::ranges::equal(signed_data_content_info.content_type.container,
		asn1::crypto::pkcs7::authenticode::oid_spc_indirect_data_content))
	{
		errors.add_error(authenticode_format_validator_errc::invalid_content_info_oid);
	}

	const auto& type_value = signed_data_content_info.content.type_value.value;
	if (!std::ranges::equal(type_value.type.container,
		asn1::crypto::pkcs7::authenticode::oid_spc_pe_image_data))
	{
		errors.add_error(authenticode_format_validator_errc::invalid_type_value_type);
	}

	if (!algorithm_id_equals(
		signed_data_content_info.content.digest.value.digest_algorithm,
		content_info.data.signer_infos[0].digest_algorithm))
	{
		errors.add_error(
			authenticode_format_validator_errc::non_matching_type_value_digest_algorithm);
	}
}

template<typename RangeType>
void validate_autenticode_format(const pkcs7::attribute_map<RangeType>& authenticated_attributes,
	std::optional<asn1::utc_time>& signing_time, error_list& errors)
{
	std::optional<asn1::crypto::object_identifier_type> content_type;
	pkcs7::validate_authenticated_attributes(authenticated_attributes,
		signing_time, content_type, errors);

	if (content_type && !std::ranges::equal(content_type->container,
		asn1::crypto::pkcs7::authenticode::oid_spc_indirect_data_content))
	{
		errors.add_error(pkcs7::pkcs7_format_validator_errc::invalid_content_type);
	}
}

template void validate_autenticode_format<span_range_type>(
	const authenticode_pkcs7<span_range_type>& signature,
	error_list& errors);
template void validate_autenticode_format<vector_range_type>(
	const authenticode_pkcs7<vector_range_type>& signature,
	error_list& errors);

template void validate_autenticode_format<span_range_type>(
	const pkcs7::attribute_map<span_range_type>& authenticated_attributes,
	std::optional<asn1::utc_time>& signing_time, error_list& errors);
template void validate_autenticode_format<vector_range_type>(
	const pkcs7::attribute_map<vector_range_type>& authenticated_attributes,
	std::optional<asn1::utc_time>& signing_time, error_list& errors);

} //namespace pe_bliss::security
