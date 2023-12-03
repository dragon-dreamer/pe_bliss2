#include "pe_bliss2/security/crypto_algorithms.h"

#include "gtest/gtest.h"

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/pkcs7/signer_info.h"

#include "simple_asn1/crypto/algorithms.h"

#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::security;

TEST(CryptoAlgorithmsTests, GetDigestAlgorithm)
{
	ASSERT_EQ(get_digest_algorithm({}), digest_algorithm::unknown);
	ASSERT_EQ(get_digest_algorithm(asn1::crypto::hash::id_sha512), digest_algorithm::sha512);
	ASSERT_EQ(get_digest_algorithm(asn1::crypto::hash::id_md5), digest_algorithm::md5);
}

TEST(CryptoAlgorithmsTests, GetDigestEncryptionAlgorithm)
{
	ASSERT_EQ(get_digest_encryption_algorithm({}), encryption_and_hash_algorithm{});

	ASSERT_EQ(get_digest_encryption_algorithm(asn1::crypto::pki::id_rsa),
		encryption_and_hash_algorithm{ .encryption_alg = digest_encryption_algorithm::rsa });
	ASSERT_EQ(get_digest_encryption_algorithm(asn1::crypto::pki::id_dsa),
		encryption_and_hash_algorithm{ .encryption_alg = digest_encryption_algorithm::dsa });
	ASSERT_EQ(get_digest_encryption_algorithm(asn1::crypto::pki::id_sha512_with_rsa),
		(encryption_and_hash_algorithm{ .encryption_alg = digest_encryption_algorithm::rsa,
			.hash_alg = digest_algorithm::sha512 }));
	ASSERT_EQ(get_digest_encryption_algorithm(asn1::crypto::pki::id_md5_with_rsa),
		(encryption_and_hash_algorithm{ .encryption_alg = digest_encryption_algorithm::rsa,
			.hash_alg = digest_algorithm::md5 }));
}

TEST(CryptoAlgorithmsTests, AlgorithmIdEqualsVector)
{
	asn1::crypto::algorithm_identifier<vector_range_type> alg1{
		.algorithm = { .container = { 1, 2, 3 } },
		.parameters = vector_range_type{
			std::byte{4}, std::byte{5}, std::byte{6}, std::byte{7} }
	};
	asn1::crypto::algorithm_identifier<vector_range_type> alg2{
		.algorithm = { .container = { 1, 2, 3, 4 } },
		.parameters = vector_range_type{
			std::byte{4}, std::byte{5}, std::byte{6}, std::byte{7} }
	};
	asn1::crypto::algorithm_identifier<vector_range_type> alg3{
		.algorithm = { .container = { 1, 2, 3 } },
		.parameters = vector_range_type{
			std::byte{5}, std::byte{5}, std::byte{6}, std::byte{7} }
	};
	ASSERT_TRUE(algorithm_id_equals(alg1, alg1));
	ASSERT_FALSE(algorithm_id_equals(alg1, alg2));
	ASSERT_FALSE(algorithm_id_equals(alg1, alg3));
	ASSERT_FALSE(algorithm_id_equals(alg3, alg2));
}

TEST(CryptoAlgorithmsTests, AlgorithmIdEqualsSpan)
{
	vector_range_type vec1{ std::byte{4}, std::byte{5}, std::byte{6}, std::byte{7} };
	vector_range_type vec2{ std::byte{5}, std::byte{5}, std::byte{6}, std::byte{7} };
	asn1::crypto::algorithm_identifier<span_range_type> alg1{
		.algorithm = { .container = { 1, 2, 3 } },
		.parameters = vec1
	};
	asn1::crypto::algorithm_identifier<span_range_type> alg2{
		.algorithm = { .container = { 1, 2, 3, 4 } },
		.parameters = vec1
	};
	asn1::crypto::algorithm_identifier<span_range_type> alg3{
		.algorithm = { .container = { 1, 2, 3 } },
		.parameters = vec2
	};
	ASSERT_TRUE(algorithm_id_equals(alg1, alg1));
	ASSERT_FALSE(algorithm_id_equals(alg1, alg2));
	ASSERT_FALSE(algorithm_id_equals(alg1, alg3));
	ASSERT_FALSE(algorithm_id_equals(alg3, alg2));
}

TEST(GetHashAndSignatureAlgorithmsTest, BothInvalid)
{
	digest_algorithm digest_alg;
	digest_encryption_algorithm digest_encryption_alg;
	pkcs7::signer_info_pkcs7<vector_range_type> signer_info{};
	pe_bliss::error_list errors;
	ASSERT_FALSE(get_hash_and_signature_algorithms(signer_info, digest_alg,
		digest_encryption_alg, errors));

	expect_contains_errors(errors,
		crypto_algorithm_errc::unsupported_digest_algorithm,
		crypto_algorithm_errc::unsupported_digest_encryption_algorithm);
	ASSERT_EQ(digest_alg, digest_algorithm::unknown);
	ASSERT_EQ(digest_encryption_alg, digest_encryption_algorithm::unknown);
}

TEST(GetHashAndSignatureAlgorithmsTest, DigestAlgoInvalid)
{
	digest_algorithm digest_alg;
	digest_encryption_algorithm digest_encryption_alg;
	pkcs7::signer_info_pkcs7<vector_range_type> signer_info{};
	signer_info.get_underlying().digest_algorithm.algorithm.container.assign(
		asn1::crypto::hash::id_sha256.begin(),
		asn1::crypto::hash::id_sha256.end());
	pe_bliss::error_list errors;
	ASSERT_FALSE(get_hash_and_signature_algorithms(signer_info, digest_alg,
		digest_encryption_alg, errors));

	expect_contains_errors(errors,
		crypto_algorithm_errc::unsupported_digest_encryption_algorithm);
	ASSERT_EQ(digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(digest_encryption_alg, digest_encryption_algorithm::unknown);
}

TEST(GetHashAndSignatureAlgorithmsTest, Valid)
{
	digest_algorithm digest_alg;
	digest_encryption_algorithm digest_encryption_alg;
	pkcs7::signer_info_pkcs7<vector_range_type> signer_info{};
	signer_info.get_underlying().digest_algorithm.algorithm.container.assign(
		asn1::crypto::hash::id_sha256.begin(),
		asn1::crypto::hash::id_sha256.end());
	signer_info.get_underlying().digest_encryption_algorithm.algorithm.container.assign(
		asn1::crypto::pki::id_rsa.begin(),
		asn1::crypto::pki::id_rsa.end());
	pe_bliss::error_list errors;
	ASSERT_TRUE(get_hash_and_signature_algorithms(signer_info, digest_alg,
		digest_encryption_alg, errors));

	expect_contains_errors(errors);
	ASSERT_EQ(digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(digest_encryption_alg, digest_encryption_algorithm::rsa);
}

TEST(GetHashAndSignatureAlgorithmsTest, ValidMismatch)
{
	digest_algorithm digest_alg;
	digest_encryption_algorithm digest_encryption_alg;
	pkcs7::signer_info_pkcs7<vector_range_type> signer_info{};
	signer_info.get_underlying().digest_algorithm.algorithm.container.assign(
		asn1::crypto::hash::id_sha256.begin(),
		asn1::crypto::hash::id_sha256.end());
	signer_info.get_underlying().digest_encryption_algorithm.algorithm.container.assign(
		asn1::crypto::signature::id_sha384_with_rsa_encryption.begin(),
		asn1::crypto::signature::id_sha384_with_rsa_encryption.end());
	pe_bliss::error_list errors;
	ASSERT_FALSE(get_hash_and_signature_algorithms(signer_info, digest_alg,
		digest_encryption_alg, errors));

	expect_contains_errors(errors,
		crypto_algorithm_errc::signature_hash_and_digest_algorithm_mismatch);
	ASSERT_EQ(digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(digest_encryption_alg, digest_encryption_algorithm::rsa);
}

TEST(GetHashAndSignatureAlgorithmsTest, ValidMatch)
{
	digest_algorithm digest_alg;
	digest_encryption_algorithm digest_encryption_alg;
	pkcs7::signer_info_pkcs7<vector_range_type> signer_info{};
	signer_info.get_underlying().digest_algorithm.algorithm.container.assign(
		asn1::crypto::hash::id_sha384.begin(),
		asn1::crypto::hash::id_sha384.end());
	signer_info.get_underlying().digest_encryption_algorithm.algorithm.container.assign(
		asn1::crypto::signature::id_sha384_with_rsa_encryption.begin(),
		asn1::crypto::signature::id_sha384_with_rsa_encryption.end());
	pe_bliss::error_list errors;
	ASSERT_TRUE(get_hash_and_signature_algorithms(signer_info, digest_alg,
		digest_encryption_alg, errors));

	expect_contains_errors(errors);
	ASSERT_EQ(digest_alg, digest_algorithm::sha384);
	ASSERT_EQ(digest_encryption_alg, digest_encryption_algorithm::rsa);
}
