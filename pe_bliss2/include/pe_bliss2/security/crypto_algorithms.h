#pragma once

#include <cstdint>
#include <optional>
#include <span>

#include "pe_bliss2/security/byte_range_types.h"

#include "simple_asn1/crypto/crypto_common_types.h"

namespace pe_bliss::security
{

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
};

[[nodiscard]]
digest_algorithm get_digest_algorithm(std::span<const std::uint32_t> range) noexcept;

[[nodiscard]]
encryption_and_hash_algorithm get_digest_encryption_algorithm(
	std::span<const std::uint32_t> range) noexcept;

bool algorithm_id_equals(const asn1::crypto::algorithm_identifier<vector_range_type>& l,
	const asn1::crypto::algorithm_identifier<vector_range_type>& r);

bool algorithm_id_equals(const asn1::crypto::algorithm_identifier<span_range_type>& l,
	const asn1::crypto::algorithm_identifier<span_range_type>& r);

} //namespace pe_bliss::security
