#pragma once

#include <algorithm>
#include <optional>
#include <system_error>
#include <type_traits>
#include <vector>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/crypto_algorithms.h"
#include "pe_bliss2/security/signature_verifier.h"
#include "pe_bliss2/security/x509/x509_certificate.h"
#include "pe_bliss2/security/x509/x509_certificate_store.h"

namespace pe_bliss::security
{

enum class authenticode_verifier_errc
{
	invalid_page_hash_format = 1,
	invalid_image_format_for_hashing
};

std::error_code make_error_code(authenticode_verifier_errc) noexcept;

template<typename RangeType>
struct [[nodiscard]] authenticode_check_status_base
{
	error_list authenticode_format_errors;
	error_list certificate_store_warnings;
	std::optional<bool> image_hash_valid;
	std::optional<bool> page_hashes_valid;
	std::error_code page_hashes_check_errc;
	std::optional<bool> message_digest_valid;
	std::optional<signature_verification_result> signature_result;
	std::optional<digest_algorithm> image_digest_alg;
	std::optional<digest_encryption_algorithm> digest_encryption_alg;
	std::optional<asn1::utc_time> signing_time;

	std::optional<x509::x509_certificate_store<
		x509::x509_certificate_ref<RangeType>>> cert_store;

	[[nodiscard]]
	explicit operator bool() const noexcept
	{
		return !authenticode_format_errors.has_errors()
			&& image_hash_valid
			&& message_digest_valid
			&& signature_result
			&& *image_hash_valid
			&& *message_digest_valid
			&& *signature_result
			&& !page_hashes_check_errc
			&& (!page_hashes_valid || *page_hashes_valid);
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

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::authenticode_verifier_errc> : true_type {};
} //namespace std
