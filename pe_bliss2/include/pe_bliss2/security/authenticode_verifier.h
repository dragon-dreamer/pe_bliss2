#pragma once

#include <algorithm>
#include <exception>
#include <optional>
#include <system_error>
#include <type_traits>
#include <variant>
#include <vector>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/authenticode_timestamp_signature.h"
#include "pe_bliss2/security/authenticode_pkcs7.h"
#include "pe_bliss2/security/crypto_algorithms.h"
#include "pe_bliss2/security/image_hash.h"
#include "pe_bliss2/security/pkcs7/pkcs7.h"
#include "pe_bliss2/security/pkcs7/pkcs7_signature.h"
#include "pe_bliss2/security/x509/x509_certificate.h"
#include "pe_bliss2/security/x509/x509_certificate_store.h"

#include "simple_asn1/types.h"

namespace pe_bliss::image { class image; }

namespace pe_bliss::security
{

enum class authenticode_verifier_errc
{
	unsupported_digest_algorithm = 1,
	unsupported_digest_encryption_algorithm,
	absent_certificates,
	duplicate_certificates,
	absent_signing_cert,
	absent_signing_cert_issuer_and_sn,
	invalid_page_hash_format,
	signature_hash_and_digest_algorithm_mismatch
};

std::error_code make_error_code(authenticode_verifier_errc) noexcept;

template<typename RangeType>
struct [[nodiscard]] authenticode_check_status_base
{
	error_list authenticode_format_errors;
	error_list certificate_store_warnings;
	std::exception_ptr authenticode_processing_error;
	std::optional<bool> image_hash_valid;
	std::optional<bool> page_hashes_valid;
	std::optional<std::error_code> page_hashes_check_errc;
	std::optional<bool> message_digest_valid;
	std::optional<bool> signature_valid;
	std::optional<digest_algorithm> image_digest_alg;
	std::optional<digest_encryption_algorithm> digest_encryption_alg;
	std::optional<asn1::utc_time> signing_time;

	std::optional<x509::x509_certificate_store<
		x509::x509_certificate_ref<RangeType>>> cert_store;

	[[nodiscard]]
	explicit operator bool() const noexcept
	{
		return !authenticode_format_errors.has_errors()
			&& !authenticode_processing_error
			&& image_hash_valid
			&& message_digest_valid
			&& signature_valid
			&& *image_hash_valid
			&& *message_digest_valid
			&& *signature_valid
			&& (!page_hashes_check_errc || !*page_hashes_check_errc)
			&& (!page_hashes_valid || *page_hashes_valid);
	}
};

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
	std::optional<bool> signature_valid;
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
			&& signature_valid
			&& *signature_valid
			&& !std::holds_alternative<std::monostate>(signing_time);
	}
};

template<typename RangeType>
struct [[nodiscard]] authenticode_check_status
{
	authenticode_check_status_base<RangeType> root;
	// Double-signing support
	std::vector<authenticode_check_status_base<RangeType>> nested;

	[[nodiscard]]
	explicit operator bool() const noexcept
	{
		return root
			&& std::ranges::all_of(nested, [](const auto& v) { return !!v; });
	}
};

struct [[nodiscard]] image_hash_verification_result
{
	bool image_hash_valid{};
	std::optional<bool> page_hashes_valid;
	std::optional<std::error_code> page_hashes_check_errc;
};

[[nodiscard]]
image_hash_verification_result verify_image_hash(span_range_type image_hash,
	digest_algorithm digest_alg, const image::image& instance,
	const std::optional<span_range_type>& page_hashes,
	const std::optional<page_hash_options>& page_hash_options);

template<typename RangeType>
[[nodiscard]]
x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>> build_certificate_store(
	const authenticode_pkcs7<RangeType>& authenticode,
	error_list* errors);
template<typename RangeType>
[[nodiscard]]
x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>> build_certificate_store(
	const authenticode_signature_cms_info_ms_bug_workaround_type<RangeType>& signature,
	error_list* warnings);
template<typename RangeType>
[[nodiscard]]
x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>> build_certificate_store(
	const authenticode_signature_cms_info_type<RangeType>& signature,
	error_list* warnings);

struct [[nodiscard]] authenticode_verification_options final
{
	page_hash_options page_hash_opts;
};

template<typename RangeType>
[[nodiscard]]
authenticode_check_status<RangeType> verify_authenticode_full(
	const authenticode_pkcs7<RangeType>& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts = {});

template<typename RangeType>
void verify_authenticode(const authenticode_pkcs7<RangeType>& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base<RangeType>& result);

// Requires cert_store to be set inside authenticode_check_status_base
template<typename RangeType>
void verify_valid_format_authenticode(
	const authenticode_pkcs7<RangeType>& authenticode,
	const pkcs7::signer_info_ref_pkcs7<RangeType>& signer,
	const pkcs7::attribute_map<RangeType>& authenticated_attributes,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base<RangeType>& result);

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
	const authenticode_signature_cms_info_ms_bug_workaround_type<RangeType>& signature);

template<typename RangeType>
bool validate_signature(
	const pkcs7::signer_info_ref_pkcs7<RangeType>& signer,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>>& cert_store,
	error_list& errors,
	std::exception_ptr& processing_error);
template<typename RangeType>
bool validate_signature(
	const pkcs7::signer_info_ref_cms<RangeType>& signer,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>>& cert_store,
	error_list& errors,
	std::exception_ptr& processing_error);

template<typename RangeType>
void verify_valid_format_timestamp_signature(
	const authenticode_signature_cms_info_ms_bug_workaround_type<RangeType>& signature,
	const pkcs7::signer_info_ref_cms<RangeType>& signer,
	const pkcs7::attribute_map<RangeType>& authenticated_attributes,
	const RangeType& authenticode_encrypted_digest,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>>& cert_store,
	timestamp_signature_check_status<RangeType>& result);

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::authenticode_verifier_errc> : true_type {};
} //namespace std
