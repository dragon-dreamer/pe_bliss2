#include "pe_bliss2/security/authenticode_timestamp_signature_verifier.h"

#include <algorithm>
#include <array>
#include <cstddef>

#include "pe_bliss2/security/authenticode_certificate_store.h"
#include "pe_bliss2/security/authenticode_timestamp_signature_format_validator.h"
#include "pe_bliss2/security/buffer_hash.h"
#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/crypto_algorithms.h"
#include "pe_bliss2/security/pkcs7/message_digest.h"
#include "pe_bliss2/security/pkcs7/pkcs7_format_validator.h"
#include "pe_bliss2/security/signature_verifier.h"

#include "simple_asn1/crypto/crypto_common_types.h"

namespace pe_bliss::security
{

template<typename RangeType>
timestamp_signature_check_status<RangeType> verify_timestamp_signature(
	const RangeType& authenticode_encrypted_digest,
	const pkcs7::signer_info_ref_pkcs7<RangeType>& timestamp_signer,
	const pkcs7::attribute_map<RangeType>& timestamp_authenticated_attributes,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>>& cert_store)
{
	timestamp_signature_check_status<RangeType> result;

	auto& digest_alg = result.digest_alg.emplace();
	auto& digest_encryption_alg = result.digest_encryption_alg.emplace();
	if (!get_hash_and_signature_algorithms(timestamp_signer,
		digest_alg, digest_encryption_alg, result.authenticode_format_errors))
	{
		return result;
	}

	std::optional<asn1::crypto::object_identifier_type> content_type;
	std::optional<asn1::utc_time> signing_time;
	pkcs7::validate_authenticated_attributes(timestamp_authenticated_attributes,
		signing_time, content_type, result.authenticode_format_errors);
	if (signing_time)
		result.signing_time = *signing_time;

	const auto message_digest = timestamp_signer.calculate_message_digest(
		std::array<span_range_type, 1u>{ authenticode_encrypted_digest });
	try
	{
		result.hash_valid = pkcs7::verify_message_digest_attribute(message_digest,
			timestamp_authenticated_attributes);
	}
	catch (const std::exception&)
	{
		result.authenticode_format_errors.add_error(
			pkcs7::pkcs7_format_validator_errc::invalid_message_digest);
		return result;
	}

	result.signature_result = verify_signature(timestamp_signer, cert_store);
	return result;
}

template<typename RangeType>
timestamp_signature_check_status<RangeType> verify_timestamp_signature(
	const RangeType& authenticode_encrypted_digest,
	const authenticode_signature_cms_info_ms_bug_workaround_type<RangeType>& signature)
{
	static constexpr std::int32_t cms_info_version = 3u;
	timestamp_signature_check_status<RangeType> result;

	validate_autenticode_timestamp_format(signature, result.authenticode_format_errors);
	if (result.authenticode_format_errors.has_errors())
		return result;

	const auto& signer = signature.get_signer(0u);

	pkcs7::attribute_map<RangeType> authenticated_attributes;
	try
	{
		authenticated_attributes = signer.get_authenticated_attributes();
	}
	catch (const pe_error& e)
	{
		result.authenticode_format_errors.add_error(e.code());
		return result;
	}

	validate_autenticode_timestamp_format(authenticated_attributes,
		result.authenticode_format_errors);
	if (result.authenticode_format_errors.has_errors())
		return result;

	result.cert_store = build_certificate_store<RangeType>(
		signature, &result.certificate_store_warnings);

	verify_valid_format_timestamp_signature(
		signature, signer, authenticated_attributes,
		authenticode_encrypted_digest,
		*result.cert_store, result);

	// signature.data.content_info.tsa points to attribute_certificate_v2_type
	result.signing_time = signature.get_content_info().data.content_info.info.value.gen_time;
	return result;
}

template<typename RangeType>
void verify_valid_format_timestamp_signature(
	const authenticode_signature_cms_info_ms_bug_workaround_type<RangeType>& signature,
	const pkcs7::signer_info_ref_cms<RangeType>& signer,
	const pkcs7::attribute_map<RangeType>& authenticated_attributes,
	const RangeType& authenticode_encrypted_digest,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>>& cert_store,
	timestamp_signature_check_status<RangeType>& result)
{
	auto& digest_alg = result.digest_alg.emplace();
	auto& digest_encryption_alg = result.digest_encryption_alg.emplace();
	if (!get_hash_and_signature_algorithms(signer,
		digest_alg, digest_encryption_alg, result.authenticode_format_errors))
	{
		return;
	}

	const auto& imprint = signature.get_content_info()
		.data.content_info.info.value.imprint;
	auto& imprint_digest_algorithm = result.imprint_digest_alg.emplace();
	imprint_digest_algorithm = get_digest_algorithm(imprint.hash_algorithm.algorithm.container);
	if (imprint_digest_algorithm == digest_algorithm::unknown)
	{
		return;
	}

	try
	{
		auto calculated_hash = calculate_hash(imprint_digest_algorithm,
			std::array<span_range_type, 1u>{ authenticode_encrypted_digest });
		result.hash_valid = std::ranges::equal(calculated_hash, imprint.hashed_message);
		result.message_digest_valid = verify_message_digest(signature,
			signer, authenticated_attributes);
	}
	catch (const std::exception&)
	{
		result.authenticode_processing_error = std::current_exception();
		return;
	}

	result.signature_result = verify_signature(signer, cert_store);
}

template void verify_valid_format_timestamp_signature<span_range_type>(
	const authenticode_signature_cms_info_ms_bug_workaround_type<span_range_type>& signature,
	const pkcs7::signer_info_ref_cms<span_range_type>& signer,
	const pkcs7::attribute_map<span_range_type>& authenticated_attributes,
	const span_range_type& authenticode_encrypted_digest,
	const x509::x509_certificate_store<x509::x509_certificate_ref<span_range_type>>& cert_store,
	timestamp_signature_check_status<span_range_type>& result);
template void verify_valid_format_timestamp_signature<vector_range_type>(
	const authenticode_signature_cms_info_ms_bug_workaround_type<vector_range_type>& signature,
	const pkcs7::signer_info_ref_cms<vector_range_type>& signer,
	const pkcs7::attribute_map<vector_range_type>& authenticated_attributes,
	const vector_range_type& authenticode_encrypted_digest,
	const x509::x509_certificate_store<x509::x509_certificate_ref<vector_range_type>>& cert_store,
	timestamp_signature_check_status<vector_range_type>& result);

template timestamp_signature_check_status<span_range_type>
verify_timestamp_signature<span_range_type>(
	const span_range_type& authenticode_encrypted_digest,
	const authenticode_signature_cms_info_ms_bug_workaround_type<span_range_type>& signature);
template timestamp_signature_check_status<vector_range_type>
verify_timestamp_signature<vector_range_type>(
	const vector_range_type& authenticode_encrypted_digest,
	const authenticode_signature_cms_info_ms_bug_workaround_type<vector_range_type>& signature);

template timestamp_signature_check_status<span_range_type>
verify_timestamp_signature<span_range_type>(
	const span_range_type& authenticode_encrypted_digest,
	const pkcs7::signer_info_ref_pkcs7<span_range_type>& timestamp_signer,
	const pkcs7::attribute_map<span_range_type>& timestamp_authenticated_attributes,
	const x509::x509_certificate_store<x509::x509_certificate_ref<span_range_type>>& cert_store);
template timestamp_signature_check_status<vector_range_type>
verify_timestamp_signature<vector_range_type>(
	const vector_range_type& authenticode_encrypted_digest,
	const pkcs7::signer_info_ref_pkcs7<vector_range_type>& timestamp_signer,
	const pkcs7::attribute_map<vector_range_type>& timestamp_authenticated_attributes,
	const x509::x509_certificate_store<x509::x509_certificate_ref<vector_range_type>>& cert_store);

} //namespace pe_bliss::security
