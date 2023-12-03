#include "pe_bliss2/security/signature_verifier.h"

#include <cstddef>
#include <vector>

#include "gtest/gtest.h"

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/pkcs7/signer_info.h"

#include "simple_asn1/crypto/algorithms.h"
#include "simple_asn1/crypto/pkcs7/cms/types.h"
#include "simple_asn1/crypto/pkcs7/types.h"

#include "tests/pe_bliss2/directories/security/hex_string_helpers.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::security;

namespace
{
template<typename T>
class SignatureVerifierTest : public testing::Test
{
public:
	using signer_info_ref_type = T;
	using range_type = typename signer_info_ref_type::range_type;
	using cert_store_type = x509::x509_certificate_store<x509::x509_certificate_ref<range_type>>;

public:
	static void set_issuer_and_sn(asn1::crypto::pkcs7::signer_info<range_type>& info)
	{
		set_issuer_and_sn(info.issuer_and_sn);
	}

	static void set_issuer_and_sn(asn1::crypto::pkcs7::cms::signer_info<range_type>& info)
	{
		auto& issuer_and_sn = info.sid.template emplace<
			asn1::crypto::pkcs7::issuer_and_serial_number<range_type>>();
		set_issuer_and_sn(issuer_and_sn);
	}

	static void set_issuer_and_sn(asn1::crypto::pkcs7::issuer_and_serial_number<range_type>& info)
	{
		info.issuer.raw = raw_issuer;
		info.serial_number = serial_number;
	}

	void add_signing_cert_to_store()
	{
		certificate.tbs_cert.issuer.raw = raw_issuer;
		certificate.tbs_cert.serial_number = serial_number;
		cert_store.add_certificate(certificate);
	}

	void set_rsa1_signer_data(bool valid_hash)
	{
		certificate.tbs_cert.pki.subject_publickey.container = valid_rsa1_public_key;
		signer_info.digest_algorithm.algorithm.container.assign(
			asn1::crypto::hash::id_sha256.begin(),
			asn1::crypto::hash::id_sha256.end());
		signer_info.digest_encryption_algorithm.algorithm.container.assign(
			asn1::crypto::pki::id_rsa.begin(),
			asn1::crypto::pki::id_rsa.end());
		signer_info.encrypted_digest = valid_rsa1_encrypted_digest;
		signer_info.authenticated_attributes.emplace().raw = valid_hash
			? valid_raw_authenticated_attributes
			: invalid_raw_authenticated_attributes;
	}

	void set_ecdsa_signature_params()
	{
		certificate.tbs_cert.pki.subject_publickey.container = valid_ecdsa_public_key;
		certificate.tbs_cert.pki.algorithm.parameters
			.emplace(valid_ecdsa_signature_params);
		signer_info.digest_algorithm.algorithm.container.assign(
			asn1::crypto::hash::id_sha256.begin(),
			asn1::crypto::hash::id_sha256.end());
		signer_info.digest_encryption_algorithm.algorithm.container.assign(
			asn1::crypto::pki::id_ec_public_key.begin(),
			asn1::crypto::pki::id_ec_public_key.end());
		signer_info.encrypted_digest = valid_ecdsa_encrypted_digest;
		signer_info.authenticated_attributes.emplace().raw = 
			valid_raw_authenticated_attributes;
	}

public:
	cert_store_type cert_store;
	typename signer_info_ref_type::signer_info_type signer_info{};
	signer_info_ref_type signer_info_ref{ signer_info };
	asn1::crypto::x509::certificate<range_type> certificate;

	static const inline auto valid_rsa1_public_key{
		hex_string_to_bytes("3048024100a6cd37607143d9057b5c433041d5f1e00dbc344a4c0e5049acfa31eb09"
		"eb269538ff0793cc016c1a1e8df7c5d36405c2307d8eb940baa62dd76bfadd8f0211e10203010001") };
	static const inline auto valid_rsa1_encrypted_digest{
		hex_string_to_bytes("2fdf71cc13395cd4ec003cdbfc7c022107c78d41eef57b69af7cd7274c5dc5c43a9"
		"ecb8c14b319cacdaedafc3667779542a4dc70d8f27877b13105e42126d51e") };
	static const inline auto valid_ecdsa_public_key{
		hex_string_to_bytes("0495fbe0145c021d37c39e1153ef6a1a2b741a09239a564f7d50bab67c980aac2b1"
		"fa6b05d1ee62d12bf465caa8ecc5d44275e9af80f283c732ec4a68e136aec82") };
	static const inline auto valid_ecdsa_encrypted_digest{
		hex_string_to_bytes("3046022100e192ff09c9c98a7776abb9ef03d0bf307876b2bb9b9e4fb4aca88fccc"
		"fa65e75022100b723b10390c9eb3bad18b27e116130f3112d3fde021f05788110bf6f5653e657") };
	static const inline auto valid_ecdsa_signature_params{
		hex_string_to_bytes("06052b8104000a") };
	static const inline std::vector valid_raw_authenticated_attributes{
		std::byte{0}, //will be replaced before hashing by SET_OF ASN.1 tag (0x31 or '1')
		std::byte{'a'}, std::byte{'b'}, std::byte{'c'}
	};
	static const inline std::vector invalid_raw_authenticated_attributes{
		std::byte{0}, //will be replaced before hashing by SET_OF ASN.1 tag (0x31 or '1')
		std::byte{'d'}, std::byte{'e'}, std::byte{'f'}
	};

	static const inline std::vector raw_issuer{
		std::byte{1}, std::byte{2}, std::byte{3}
	};
	static const inline std::vector serial_number{
		std::byte{4}, std::byte{5}, std::byte{6}, std::byte{7}
	};
};

using tested_range_types = ::testing::Types<
	pkcs7::signer_info_ref_pkcs7<vector_range_type>,
	pkcs7::signer_info_ref_pkcs7<span_range_type>,
	pkcs7::signer_info_ref_cms<vector_range_type>,
	pkcs7::signer_info_ref_cms<span_range_type>>;
} //namespace

TYPED_TEST_SUITE(SignatureVerifierTest, tested_range_types);

TYPED_TEST(SignatureVerifierTest, Empty)
{
	const auto result = verify_signature(this->signer_info_ref, this->cert_store);
	ASSERT_FALSE(result);
	ASSERT_FALSE(result.processing_error);
	ASSERT_FALSE(result.pkcs7_result);
	expect_contains_errors(result.errors,
		signature_verifier_errc::absent_signing_cert_issuer_and_sn);
}

TYPED_TEST(SignatureVerifierTest, AbsentSigningCert)
{
	this->set_issuer_and_sn(this->signer_info);

	const auto result = verify_signature(this->signer_info_ref, this->cert_store);
	ASSERT_FALSE(result);
	ASSERT_FALSE(result.processing_error);
	ASSERT_FALSE(result.pkcs7_result);
	expect_contains_errors(result.errors,
		signature_verifier_errc::absent_signing_cert);
}

TYPED_TEST(SignatureVerifierTest, InvalidSigningCertificate)
{
	this->set_issuer_and_sn(this->signer_info);
	this->add_signing_cert_to_store();

	const auto result = verify_signature(this->signer_info_ref, this->cert_store);
	ASSERT_FALSE(result);
	ASSERT_TRUE(result.processing_error);
	ASSERT_FALSE(result.pkcs7_result);
	expect_contains_errors(result.errors,
		signature_verifier_errc::unable_to_verify_signature);
}

TYPED_TEST(SignatureVerifierTest, InvalidSignature)
{
	this->set_issuer_and_sn(this->signer_info);
	this->add_signing_cert_to_store();
	this->set_rsa1_signer_data(false);

	const auto result = verify_signature(this->signer_info_ref, this->cert_store);
	ASSERT_FALSE(result);
	ASSERT_FALSE(result.processing_error);
	ASSERT_FALSE(result.pkcs7_result);
	expect_contains_errors(result.errors);
}

TYPED_TEST(SignatureVerifierTest, ValidSignature)
{
	this->set_issuer_and_sn(this->signer_info);
	this->add_signing_cert_to_store();
	this->set_rsa1_signer_data(true);

	const auto result = verify_signature(this->signer_info_ref, this->cert_store);
	ASSERT_TRUE(result);
	ASSERT_FALSE(result.processing_error);
	ASSERT_TRUE(result.pkcs7_result);
	expect_contains_errors(result.errors);
}

TYPED_TEST(SignatureVerifierTest, ValidEcdsaSignature)
{
	this->set_issuer_and_sn(this->signer_info);
	this->add_signing_cert_to_store();
	this->set_ecdsa_signature_params();

	const auto result = verify_signature(this->signer_info_ref, this->cert_store);
	ASSERT_TRUE(result);
	ASSERT_FALSE(result.processing_error);
	ASSERT_TRUE(result.pkcs7_result);
	expect_contains_errors(result.errors);
}
