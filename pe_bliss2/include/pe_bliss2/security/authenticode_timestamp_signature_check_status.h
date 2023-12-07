#pragma once

#include <optional>
#include <variant>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/crypto_algorithms.h"
#include "pe_bliss2/security/signature_verifier.h"
#include "pe_bliss2/security/x509/x509_certificate.h"
#include "pe_bliss2/security/x509/x509_certificate_store.h"

#include "simple_asn1/types.h"

namespace pe_bliss::security
{

template<typename RangeType>
struct [[nodiscard]] authenticode_timestamp_signature_check_status
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

} //namespace pe_bliss::security