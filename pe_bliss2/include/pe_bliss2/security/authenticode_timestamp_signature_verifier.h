#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <optional>
#include <variant>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/authenticode_certificate_store.h"
#include "pe_bliss2/security/authenticode_timestamp_signature.h"
#include "pe_bliss2/security/authenticode_timestamp_signature_format_validator.h"
#include "pe_bliss2/security/buffer_hash.h"
#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/crypto_algorithms.h"
#include "pe_bliss2/security/pkcs7/attribute_map.h"
#include "pe_bliss2/security/pkcs7/message_digest.h"
#include "pe_bliss2/security/pkcs7/pkcs7_format_validator.h"
#include "pe_bliss2/security/pkcs7/signer_info.h"
#include "pe_bliss2/security/signature_verifier.h"
#include "pe_bliss2/security/x509/x509_certificate.h"
#include "pe_bliss2/security/x509/x509_certificate_store.h"

#include "simple_asn1/types.h"

#include "utilities/variant_helpers.h"

namespace pe_bliss::security
{

template<typename RangeType>
struct [[nodiscard]] timestamp_signature_check_status
{
	error_list authenticode_format_errors;
	error_list certificate_store_warnings;
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
			&& hash_valid
			&& *hash_valid
			&& (!message_digest_valid || *message_digest_valid)
			&& signature_result
			&& *signature_result
			&& !std::holds_alternative<std::monostate>(signing_time);
	}
};

namespace impl
{
template<typename Signature, typename RangeType1, typename RangeType2,
	typename RangeType3, typename RangeType4, typename RangeType5>
void verify_valid_format_timestamp_signature_impl(
	const Signature& signature,
	const pkcs7::signer_info_ref_cms<RangeType1>& signer,
	const pkcs7::attribute_map<RangeType2>& authenticated_attributes,
	const RangeType3& authenticode_encrypted_digest,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType4>>& cert_store,
	timestamp_signature_check_status<RangeType5>& result)
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
		result.authenticode_format_errors.add_error(
			crypto_algorithm_errc::unsupported_digest_algorithm);
		return;
	}

	const auto calculated_hash = calculate_hash(imprint_digest_algorithm,
		std::array<span_range_type, 1u>{ authenticode_encrypted_digest });
	result.hash_valid = std::ranges::equal(calculated_hash, imprint.hashed_message);

	try
	{
		result.message_digest_valid = verify_message_digest(signature,
			signer, authenticated_attributes);
	}
	catch (const std::exception&)
	{
		result.authenticode_format_errors.add_error(
			pkcs7::pkcs7_format_validator_errc::invalid_message_digest);
		return;
	}

	result.signature_result = verify_signature(signer, cert_store);
}

template<typename RangeType1, typename RangeType2, typename Signature>
timestamp_signature_check_status<RangeType1> verify_timestamp_signature_impl(
	const RangeType2& authenticode_encrypted_digest,
	const Signature& signature)
{
	static constexpr std::int32_t cms_info_version = 3u;
	timestamp_signature_check_status<RangeType1> result;

	validate_autenticode_timestamp_format(signature, result.authenticode_format_errors);
	if (result.authenticode_format_errors.has_errors())
		return result;

	const auto& signer = signature.get_signer(0u);

	decltype(signer.get_authenticated_attributes()) authenticated_attributes;
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

	result.cert_store = build_certificate_store<RangeType1>(
		signature, &result.certificate_store_warnings);

	verify_valid_format_timestamp_signature_impl(
		signature, signer, authenticated_attributes,
		authenticode_encrypted_digest,
		*result.cert_store, result);

	// signature.data.content_info.tsa points to attribute_certificate_v2_type
	result.signing_time = signature.get_content_info().data.content_info.info.value.gen_time;
	return result;
}
} //namespace impl

template<typename RangeType1 = span_range_type, typename RangeType2,
	typename RangeType3, typename RangeType4, typename RangeType5>
timestamp_signature_check_status<RangeType1> verify_timestamp_signature(
	const RangeType2& authenticode_encrypted_digest,
	const pkcs7::signer_info_ref_pkcs7<RangeType3>& timestamp_signer,
	const pkcs7::attribute_map<RangeType4>& timestamp_authenticated_attributes,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType5>>& cert_store)
{
	timestamp_signature_check_status<RangeType1> result;

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

template<typename RangeType1 = span_range_type, typename RangeType2,
	typename RangeType3, typename RangeType4, typename RangeType5>
timestamp_signature_check_status<RangeType1> verify_timestamp_signature(
	const RangeType2& authenticode_encrypted_digest,
	const pkcs7::signer_info_pkcs7<RangeType3>& timestamp_signer,
	const pkcs7::attribute_map<RangeType4>& timestamp_authenticated_attributes,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType5>>& cert_store) {
	return verify_timestamp_signature<RangeType1>(authenticode_encrypted_digest,
		pkcs7::signer_info_ref_pkcs7(timestamp_signer),
		timestamp_authenticated_attributes,
		cert_store);
}

template<typename RangeType1 = span_range_type, typename RangeType2>
timestamp_signature_check_status<RangeType1> verify_timestamp_signature(
	const RangeType2& authenticode_encrypted_digest,
	const authenticode_signature_cms_info_ms_bug_workaround_type<RangeType1>& signature)
{
	return impl::verify_timestamp_signature_impl<RangeType1>(
		authenticode_encrypted_digest, signature);
}

template<typename RangeType1 = span_range_type, typename RangeType2>
timestamp_signature_check_status<RangeType1> verify_timestamp_signature(
	const RangeType2& authenticode_encrypted_digest,
	const authenticode_signature_cms_info_type<RangeType1>& signature)
{
	return impl::verify_timestamp_signature_impl<RangeType1>(
		authenticode_encrypted_digest, signature);
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
	impl::verify_valid_format_timestamp_signature_impl(signature,
		signer, authenticated_attributes, authenticode_encrypted_digest,
		cert_store, result);
}

template<typename RangeType>
void verify_valid_format_timestamp_signature(
	const authenticode_signature_cms_info_type<RangeType>& signature,
	const pkcs7::signer_info_ref_cms<RangeType>& signer,
	const pkcs7::attribute_map<RangeType>& authenticated_attributes,
	const RangeType& authenticode_encrypted_digest,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>>& cert_store,
	timestamp_signature_check_status<RangeType>& result)
{
	impl::verify_valid_format_timestamp_signature_impl(signature,
		signer, authenticated_attributes, authenticode_encrypted_digest,
		cert_store, result);
}

template<typename RangeType1, typename RangeType2, typename RangeType3,
	typename RangeType4>
timestamp_signature_check_status<RangeType1> verify_timestamp_signature(
	const RangeType2& authenticode_encrypted_digest,
	const pe_bliss::security::authenticode_timestamp_signature<RangeType3>& signature,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType4>>& cert_store)
{
	using ts_sign_type = pe_bliss::security
		::authenticode_timestamp_signature<RangeType3>;
	return std::visit(utilities::overloaded(
		[&authenticode_encrypted_digest, &cert_store](
			const typename ts_sign_type::signer_info_type& sign) {
			return verify_timestamp_signature<RangeType1>(
				authenticode_encrypted_digest, sign,
				sign.get_authenticated_attributes(), cert_store);
		},
		[&authenticode_encrypted_digest](const auto& sign) {
			return verify_timestamp_signature<RangeType1>(
				authenticode_encrypted_digest, sign);
		}
	), signature.get_underlying_type());
}

template<typename RangeType1, typename RangeType2, typename RangeType3,
	typename RangeType4>
std::optional<timestamp_signature_check_status<RangeType1>> verify_timestamp_signature(
	const RangeType2& authenticode_encrypted_digest,
	const pkcs7::attribute_map<RangeType3>& unauthenticated_attributes,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType4>>& cert_store)
{
	const auto signature = pe_bliss::security::load_timestamp_signature<span_range_type>(
		unauthenticated_attributes);
	if (!signature)
		return {};

	return verify_timestamp_signature<RangeType1>(authenticode_encrypted_digest,
		*signature, cert_store);
}

template<typename RangeType1, typename RangeType2, typename RangeType3>
std::optional<timestamp_signature_check_status<RangeType1>> verify_timestamp_signature(
	const authenticode_pkcs7<RangeType2>& authenticode,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType3>>& cert_store)
{
	const auto& signer = authenticode.get_signer(0);
	return verify_timestamp_signature<RangeType1>(signer.get_encrypted_digest(),
		signer.get_unauthenticated_attributes(), cert_store);
}

} //namespace pe_bliss::security
