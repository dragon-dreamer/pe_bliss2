#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/byte_range_types.h"

#include "simple_asn1/crypto/crypto_common_types.h"

namespace pe_bliss::security
{

enum class crypto_algorithm_errc
{
	unsupported_digest_algorithm = 1,
	unsupported_digest_encryption_algorithm,
	signature_hash_and_digest_algorithm_mismatch
};

std::error_code make_error_code(crypto_algorithm_errc) noexcept;

enum class digest_algorithm
{
	md5,
	sha1,
	sha256,
	sha384,
	sha512,
	unknown
};

enum class digest_encryption_algorithm
{
	rsa,
	dsa,
	ecdsa,
	unknown
};

struct encryption_and_hash_algorithm
{
	digest_encryption_algorithm encryption_alg{ digest_encryption_algorithm::unknown };
	std::optional<digest_algorithm> hash_alg;
	friend bool operator==(const encryption_and_hash_algorithm&,
		const encryption_and_hash_algorithm&) noexcept = default;
};

[[nodiscard]]
digest_algorithm get_digest_algorithm(std::span<const std::uint32_t> range) noexcept;

[[nodiscard]]
encryption_and_hash_algorithm get_digest_encryption_algorithm(
	std::span<const std::uint32_t> range) noexcept;

bool algorithm_id_equals(const asn1::crypto::algorithm_identifier<vector_range_type>& l,
	const asn1::crypto::algorithm_identifier<vector_range_type>& r) noexcept;

bool algorithm_id_equals(const asn1::crypto::algorithm_identifier<span_range_type>& l,
	const asn1::crypto::algorithm_identifier<span_range_type>& r) noexcept;

template<typename Signer>
bool get_hash_and_signature_algorithms(
	const Signer& signer,
	digest_algorithm& digest_alg,
	digest_encryption_algorithm& digest_encryption_alg,
	error_list& errors)
{
	digest_alg = signer.get_digest_algorithm();
	const auto [signature_alg, digest_alg_from_encryption]
		= signer.get_digest_encryption_algorithm();
	digest_encryption_alg = signature_alg;
	bool valid = true;
	if (digest_alg == digest_algorithm::unknown)
	{
		errors.add_error(crypto_algorithm_errc::unsupported_digest_algorithm);
		valid = false;
	}

	if (digest_encryption_alg == digest_encryption_algorithm::unknown)
	{
		errors.add_error(crypto_algorithm_errc::unsupported_digest_encryption_algorithm);
		valid = false;
	}

	if (digest_alg_from_encryption && digest_alg_from_encryption != digest_alg)
	{
		errors.add_error(
			crypto_algorithm_errc::signature_hash_and_digest_algorithm_mismatch);
		valid = false;
	}
	return valid;
}

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::crypto_algorithm_errc> : true_type {};
} //namespace std
