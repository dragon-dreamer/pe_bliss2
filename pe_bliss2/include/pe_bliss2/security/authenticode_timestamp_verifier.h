#pragma once

#include <exception>
#include <optional>
#include <variant>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/authenticode_timestamp_signature.h"
#include "pe_bliss2/security/crypto_algorithms.h"
#include "pe_bliss2/security/pkcs7/attribute_map.h"
#include "pe_bliss2/security/pkcs7/signer_info.h"
#include "pe_bliss2/security/signature_verifier.h"
#include "pe_bliss2/security/x509/x509_certificate.h"
#include "pe_bliss2/security/x509/x509_certificate_store.h"

#include "simple_asn1/types.h"

namespace pe_bliss::security
{

template<typename RangeType>
struct [[nodiscard]] timestamp_signature_check_status
{
	error_list authenticode_format_errors;
	error_list certificate_store_warnings;
	std::exception_ptr authenticode_processing_error;
	std::optional<bool> hash_valid;
	std::optional<bool> message_digest_valid;
	std::optional<digest_algorithm> digest_alg;
	std::optional<digest_algorithm> imprint_digest_alg;
	std::optional<digest_encryption_algorithm> digest_encryption_alg;
	std::optional<signature_verification_result> signature_result;
	std::variant<std::monostate, asn1::utc_time, asn1::generalized_time> signing_time;

	std::optional<x509::x509_certificate_store<
		x509::x509_certificate_ref<RangeType>>> cert_store;

	[[nodiscard]]
	explicit operator bool() const noexcept
	{
		return !authenticode_format_errors.has_errors()
			&& !authenticode_processing_error
			&& hash_valid
			&& *hash_valid
			&& (!message_digest_valid || *message_digest_valid)
			&& signature_result
			&& *signature_result
			&& !std::holds_alternative<std::monostate>(signing_time);
	}
};

template<typename RangeType>
[[nodiscard]]
timestamp_signature_check_status<RangeType> verify_timestamp_signature(
	const RangeType& authenticode_encrypted_digest,
	const pkcs7::signer_info_ref_pkcs7<RangeType>& timestamp_signer,
	const pkcs7::attribute_map<RangeType>& timestamp_authenticated_attributes,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>>& cert_store);

template<typename RangeType>
[[nodiscard]]
timestamp_signature_check_status<RangeType> verify_timestamp_signature(
	const RangeType& authenticode_encrypted_digest,
	const pkcs7::signer_info_pkcs7<RangeType>& timestamp_signer,
	const pkcs7::attribute_map<RangeType>& timestamp_authenticated_attributes,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>>& cert_store) {
	return verify_timestamp_signature(authenticode_encrypted_digest,
		pkcs7::signer_info_ref_pkcs7(timestamp_signer),
		timestamp_authenticated_attributes,
		cert_store);
}

template<typename RangeType>
[[nodiscard]]
timestamp_signature_check_status<RangeType> verify_timestamp_signature(
	const RangeType& authenticode_encrypted_digest,
	const authenticode_signature_cms_info_ms_bug_workaround_type<RangeType>& signature);

template<typename RangeType>
void verify_valid_format_timestamp_signature(
	const authenticode_signature_cms_info_ms_bug_workaround_type<RangeType>& signature,
	const pkcs7::signer_info_ref_cms<RangeType>& signer,
	const pkcs7::attribute_map<RangeType>& authenticated_attributes,
	const RangeType& authenticode_encrypted_digest,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>>& cert_store,
	timestamp_signature_check_status<RangeType>& result);

} //namespace pe_bliss::security
