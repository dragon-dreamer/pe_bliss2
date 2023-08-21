#include "pe_bliss2/security/crypto_algorithms.h"

#include <algorithm>
#include <array>

#include "simple_asn1/crypto/algorithms.h"

namespace pe_bliss::security
{

digest_algorithm get_digest_algorithm(std::span<const std::uint32_t> range) noexcept
{
	if (std::ranges::equal(range, asn1::crypto::hash::id_sha256))
		return digest_algorithm::sha256;
	if (std::ranges::equal(range, asn1::crypto::hash::id_sha384))
		return digest_algorithm::sha384;
	if (std::ranges::equal(range, asn1::crypto::hash::id_sha512))
		return digest_algorithm::sha512;
	if (std::ranges::equal(range, asn1::crypto::hash::id_sha1))
		return digest_algorithm::sha1;
	if (std::ranges::equal(range, asn1::crypto::hash::id_md5))
		return digest_algorithm::md5;

	return digest_algorithm::unknown;
}

encryption_and_hash_algorithm get_digest_encryption_algorithm(std::span<const std::uint32_t> range) noexcept
{
	if (std::ranges::equal(range, asn1::crypto::pki::id_rsa))
		return { digest_encryption_algorithm::rsa };
	if (std::ranges::equal(range, asn1::crypto::pki::id_dsa))
		return { digest_encryption_algorithm::dsa };
	if (std::ranges::equal(range, asn1::crypto::pki::id_ec_public_key))
		return { digest_encryption_algorithm::ecdsa };

	if (std::ranges::equal(range, asn1::crypto::pki::id_sha256_with_rsa))
		return { digest_encryption_algorithm::rsa, digest_algorithm::sha256 };
	if (std::ranges::equal(range, asn1::crypto::pki::id_md5_with_rsa))
		return { digest_encryption_algorithm::rsa, digest_algorithm::md5 };
	if (std::ranges::equal(range, asn1::crypto::pki::id_sha1_with_rsa))
		return { digest_encryption_algorithm::rsa, digest_algorithm::sha1 };
	if (std::ranges::equal(range, asn1::crypto::pki::id_sha384_with_rsa))
		return { digest_encryption_algorithm::rsa, digest_algorithm::sha384 };
	if (std::ranges::equal(range, asn1::crypto::pki::id_sha512_with_rsa))
		return { digest_encryption_algorithm::rsa, digest_algorithm::sha512 };

	return { digest_encryption_algorithm::unknown };
}

bool algorithm_id_equals(const asn1::crypto::algorithm_identifier<vector_range_type>& l,
	const asn1::crypto::algorithm_identifier<vector_range_type>& r)
{
	return l.algorithm == r.algorithm
		&& l.parameters == r.parameters;
}

bool algorithm_id_equals(const asn1::crypto::algorithm_identifier<span_range_type>& l,
	const asn1::crypto::algorithm_identifier<span_range_type>& r)
{
	if (l.algorithm != r.algorithm)
		return false;

	if (l.parameters.has_value() != r.parameters.has_value())
		return false;

	if (l.parameters.has_value())
		return std::ranges::equal(*l.parameters, *r.parameters);

	return true;
}

} //namespace pe_bliss::security
