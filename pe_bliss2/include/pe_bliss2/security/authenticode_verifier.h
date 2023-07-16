#pragma once

#include <algorithm>
#include <exception>
#include <optional>
#include <system_error>
#include <type_traits>
#include <variant>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/authenticode_format_validator.h"
#include "pe_bliss2/security/authenticode_pkcs7.h"
#include "pe_bliss2/security/crypto_algorithms.h"
#include "pe_bliss2/security/image_hash.h"
#include "pe_bliss2/security/pkcs7/pkcs7.h"
#include "pe_bliss2/security/pkcs7/pkcs7_signature.h"
#include "pe_bliss2/security/x509/x509_certificate.h"
#include "pe_bliss2/security/x509/x509_certificate_store.h"

namespace pe_bliss::image { class image; }

namespace pe_bliss::security
{

enum class authenticode_verifier_errc
{
	unsupported_digest_algorithm,
	unsupported_digest_encryption_algorithm,
	absent_certificates,
	duplicate_certificates,
	absent_signing_cert,
	invalid_page_hash_format
};

std::error_code make_error_code(authenticode_verifier_errc) noexcept;

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

struct [[nodiscard]] authenticode_check_status
{
	authenticode_check_status_base root;
	// Double-signing support
	std::vector<authenticode_check_status_base> nested;

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
bool verify_message_digest(const authenticode_pkcs7<RangeType>& authenticode,
	const pkcs7::signer_info_ref<RangeType>& signer,
	const pkcs7::attribute_map<RangeType>& authenticated_attributes);

template<typename RangeType>
[[nodiscard]]
x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>> build_certificate_store(
	const authenticode_pkcs7<RangeType>& authenticode,
	error_list* errors);

struct [[nodiscard]] authenticode_verification_options final
{
	page_hash_options page_hash_opts;
};

template<typename RangeType>
[[nodiscard]]
authenticode_check_status verify_authenticode_full(const authenticode_pkcs7<RangeType>& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts = {});

template<typename RangeType>
void verify_authenticode(const authenticode_pkcs7<RangeType>& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base& result);

template<typename RangeType>
void verify_valid_format_authenticode(
	const authenticode_pkcs7<RangeType>& authenticode,
	const pkcs7::signer_info_ref<RangeType>& signer,
	const pkcs7::attribute_map<RangeType>& authenticated_attributes,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base& result);

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::authenticode_verifier_errc> : true_type {};
} //namespace std
