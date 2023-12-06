#include "pe_bliss2/security/authenticode_timestamp_signature_verifier.h"

#include <variant>

#include "gtest/gtest.h"

#include "pe_bliss2/security/authenticode_format_validator_errc.h"
#include "pe_bliss2/security/authenticode_timestamp_signature_format_validator.h"
#include "pe_bliss2/security/authenticode_certificate_store.h"
#include "pe_bliss2/security/crypto_algorithms.h"
#include "pe_bliss2/security/pkcs7/attribute_map.h"
#include "pe_bliss2/security/pkcs7/pkcs7_format_validator.h"
#include "pe_bliss2/security/pkcs7/signer_info.h"
#include "pe_bliss2/security/signature_verifier.h"
#include "pe_bliss2/security/x509/x509_certificate.h"
#include "pe_bliss2/security/x509/x509_certificate_store.h"

#include "simple_asn1/crypto/algorithms.h"
#include "simple_asn1/crypto/pkcs7/oids.h"
#include "simple_asn1/crypto/pkcs9/oids.h"

#include "tests/pe_bliss2/directories/security/hex_string_helpers.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::security;

namespace
{

template<typename RangeType>
class AuthenticodeTimestampSignatureVerifierTestBase : public testing::Test
{
public:
	template<typename Attributes>
	void init_authenticated_attributes(Attributes& authenticated_attributes,
		const std::vector<RangeType>& message_digest)
	{
		authenticated_attributes.get_map().try_emplace(
			oid_message_digest, message_digest);

		authenticated_attributes.get_map().try_emplace(
			oid_content_type, this->valid_oid_data);

		authenticated_attributes.get_map().try_emplace(
			oid_signing_time, this->signing_time_data);
	}

	void init_authenticated_attributes(
		std::vector<asn1::crypto::pkcs7::attribute<RangeType>>& authenticated_attributes,
		const std::vector<RangeType>& message_digest)
	{
		authenticated_attributes.emplace_back(
			asn1::crypto::pkcs7::attribute<RangeType>{
			.type = asn1::decoded_object_identifier{
				.container = oid_message_digest
			},
			.values = message_digest
		});

		authenticated_attributes.emplace_back(
			asn1::crypto::pkcs7::attribute<RangeType>{
			.type = asn1::decoded_object_identifier{
				.container = oid_content_type
			},
			.values = this->valid_oid_data
		});
	}

public:
	static const inline std::vector valid_oid{ std::byte{0x06u}, std::byte{0x0bu},
		std::byte{0x2au}, std::byte{0x86u}, std::byte{0x48u}, std::byte{0x86u},
		std::byte{0xf7u}, std::byte{0x0du}, std::byte{0x01u}, std::byte{0x09u},
		std::byte{0x10u}, std::byte{0x01u}, std::byte{0x04u} };
	static const inline std::vector<RangeType> valid_oid_data{ valid_oid };

	static const inline std::vector<std::byte> valid_utc_time{
		std::byte{0x17u}, std::byte{0x0du},
		std::byte{0x39u}, std::byte{0x36u}, std::byte{0x30u}, std::byte{0x34u},
		std::byte{0x31u}, std::byte{0x35u}, std::byte{0x32u}, std::byte{0x30u},
		std::byte{0x33u}, std::byte{0x30u}, std::byte{0x30u}, std::byte{0x31u},
		std::byte{0x5au} };
	static const inline std::vector<RangeType> signing_time_data{ valid_utc_time };

	static const inline std::vector<std::uint32_t> oid_message_digest{
		asn1::crypto::pkcs7::oid_message_digest.cbegin(),
		asn1::crypto::pkcs7::oid_message_digest.cend() };
	static const inline std::vector<std::uint32_t> oid_content_type{
		asn1::crypto::pkcs7::oid_content_type.cbegin(),
		asn1::crypto::pkcs7::oid_content_type.cend() };
	static const inline std::vector<std::uint32_t> oid_signing_time{
		asn1::crypto::pkcs7::oid_signing_time.cbegin(),
		asn1::crypto::pkcs7::oid_signing_time.cend() };

	static const inline std::vector valid_message_digest{
		std::byte{0x04u}, std::byte{0x02u},
		std::byte{1}, std::byte{2} };
	static const inline std::vector<RangeType> valid_message_digest_data{
		valid_message_digest };
	static const inline std::vector invalid_message_digest{
		std::byte{1}, std::byte{2} };
	static const inline std::vector<RangeType> invalid_message_digest_data{
		invalid_message_digest };
	static const inline std::vector valid_and_correct_message_digest{
		std::byte{0x04u}, std::byte{0x20u},
		std::byte{0xfbu}, std::byte{0x8eu}, std::byte{0x20u}, std::byte{0xfcu},
		std::byte{0x2eu}, std::byte{0x4cu}, std::byte{0x3fu}, std::byte{0x24u},
		std::byte{0x8cu}, std::byte{0x60u}, std::byte{0xc3u}, std::byte{0x9bu},
		std::byte{0xd6u}, std::byte{0x52u}, std::byte{0xf3u}, std::byte{0xc1u},
		std::byte{0x34u}, std::byte{0x72u}, std::byte{0x98u}, std::byte{0xbbu},
		std::byte{0x97u}, std::byte{0x7bu}, std::byte{0x8bu}, std::byte{0x4du},
		std::byte{0x59u}, std::byte{0x03u}, std::byte{0xb8u}, std::byte{0x50u},
		std::byte{0x55u}, std::byte{0x62u}, std::byte{0x06u}, std::byte{0x03u} };
	static const inline std::vector<RangeType> valid_and_correct_message_digest_data{
		valid_and_correct_message_digest };

	static const inline std::vector raw_issuer{
		std::byte{1}, std::byte{2}, std::byte{3}
	};
	static const inline std::vector serial_number{
		std::byte{4}, std::byte{5}, std::byte{6}, std::byte{7}
	};

	static const inline auto valid_rsa1_public_key{
		hex_string_to_bytes("3048024100a6cd37607143d9057b5c433041d5f1e00dbc344a4c0e5049acfa31eb09"
		"eb269538ff0793cc016c1a1e8df7c5d36405c2307d8eb940baa62dd76bfadd8f0211e10203010001") };

	static const inline auto valid_rsa1_encrypted_digest{
		hex_string_to_bytes("2fdf71cc13395cd4ec003cdbfc7c022107c78d41eef57b69af7cd7274c5dc5c43a9"
		"ecb8c14b319cacdaedafc3667779542a4dc70d8f27877b13105e42126d51e") };

	static const inline std::vector valid_raw_authenticated_attributes{
		std::byte{0}, //will be replaced before hashing by SET_OF ASN.1 tag (0x31 or '1')
		std::byte{'a'}, std::byte{'b'}, std::byte{'c'}
	};
	static const inline std::vector invalid_raw_authenticated_attributes{
		std::byte{0}, //will be replaced before hashing by SET_OF ASN.1 tag (0x31 or '1')
		std::byte{'d'}, std::byte{'e'}, std::byte{'f'}
	};
};

template<typename T>
class AuthenticodeTimestampPkcs7SignatureVerifierTest
	: public AuthenticodeTimestampSignatureVerifierTestBase<T>
{
public:
	using range_type = T;

public:
	auto verify() const
	{
		return verify_timestamp_signature(encrypted_digest,
			signer, authenticated_attributes, cert_store);
	}

	void init_algorithms()
	{
		signer.get_underlying().digest_algorithm.algorithm.container.assign(
			asn1::crypto::hash::id_sha256.begin(),
			asn1::crypto::hash::id_sha256.end());
		signer.get_underlying().digest_encryption_algorithm.algorithm.container.assign(
			asn1::crypto::pki::id_rsa.begin(),
			asn1::crypto::pki::id_rsa.end());
	}

	void init_authenticated_attributes(const std::vector<range_type>& message_digest)
	{
		AuthenticodeTimestampSignatureVerifierTestBase<T>
			::init_authenticated_attributes(authenticated_attributes, message_digest);
	}

	void set_issuer_and_sn(asn1::crypto::pkcs7::signer_info<range_type>& info)
	{
		set_issuer_and_sn(info.issuer_and_sn);
	}

	void set_issuer_and_sn(asn1::crypto::pkcs7::issuer_and_serial_number<range_type>& info)
	{
		info.issuer.raw = this->raw_issuer;
		info.serial_number = this->serial_number;
	}

	void init_signing_certificate()
	{
		set_issuer_and_sn(signer.get_underlying());
		signer.get_underlying().encrypted_digest = this->valid_rsa1_encrypted_digest;
		certificate.tbs_cert.issuer.raw = this->raw_issuer;
		certificate.tbs_cert.serial_number = this->serial_number;
		certificate.tbs_cert.pki.subject_publickey.container = this->valid_rsa1_public_key;
		cert_store.add_certificate(certificate);
	}

	void add_raw_authenticated_attributes(bool valid_hash)
	{
		signer.get_underlying().authenticated_attributes.emplace().raw
			= valid_hash ? this->valid_raw_authenticated_attributes
			: this->invalid_raw_authenticated_attributes;
	}

	static void check_signing_time(const timestamp_signature_check_status<span_range_type>& result)
	{
		const auto* signing_time = std::get_if<asn1::utc_time>(&result.signing_time);
		ASSERT_NE(signing_time, nullptr);
		ASSERT_EQ(signing_time->year, 96u);
		ASSERT_EQ(signing_time->month, 4u);
		ASSERT_EQ(signing_time->day, 15u);
		ASSERT_EQ(signing_time->hour, 20u);
		ASSERT_EQ(signing_time->minute, 30u);
		ASSERT_EQ(signing_time->second, 1u);
	}

public:
	pkcs7::attribute_map<range_type> authenticated_attributes;
	pkcs7::signer_info_pkcs7<range_type> signer{};
	static const inline std::vector encrypted_digest{ std::byte{'a'}, std::byte{'b'} };
	asn1::crypto::x509::certificate<range_type> certificate;
	x509::x509_certificate_store<x509::x509_certificate_ref<range_type>> cert_store;
};

using pkcs7_tested_range_types = ::testing::Types<
	vector_range_type, span_range_type>;
} // namespace

TYPED_TEST_SUITE(AuthenticodeTimestampPkcs7SignatureVerifierTest, pkcs7_tested_range_types);

TYPED_TEST(AuthenticodeTimestampPkcs7SignatureVerifierTest, Empty)
{
	const auto result = this->verify();
	ASSERT_FALSE(result);
	expect_contains_errors(result.authenticode_format_errors,
		crypto_algorithm_errc::unsupported_digest_algorithm,
		crypto_algorithm_errc::unsupported_digest_encryption_algorithm);
	expect_contains_errors(result.certificate_store_warnings);
	ASSERT_FALSE(result.hash_valid);
	ASSERT_FALSE(result.message_digest_valid);
	ASSERT_EQ(result.digest_alg, digest_algorithm::unknown);
	ASSERT_FALSE(result.imprint_digest_alg);
	ASSERT_EQ(result.digest_encryption_alg, digest_encryption_algorithm::unknown);
	ASSERT_FALSE(result.signature_result);
	ASSERT_TRUE(std::holds_alternative<std::monostate>(result.signing_time));
	ASSERT_FALSE(result.cert_store);
}

TYPED_TEST(AuthenticodeTimestampPkcs7SignatureVerifierTest, EmptyAuthenticatedAttributes)
{
	this->init_algorithms();

	const auto result = this->verify();
	ASSERT_FALSE(result);
	expect_contains_errors(result.authenticode_format_errors,
		pkcs7::pkcs7_format_validator_errc::absent_message_digest,
		pkcs7::pkcs7_format_validator_errc::absent_content_type);
	expect_contains_errors(result.certificate_store_warnings);
	ASSERT_TRUE(result.hash_valid);
	ASSERT_FALSE(*result.hash_valid);
	ASSERT_FALSE(result.message_digest_valid);
	ASSERT_EQ(result.digest_alg, digest_algorithm::sha256);
	ASSERT_FALSE(result.imprint_digest_alg);
	ASSERT_EQ(result.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_TRUE(result.signature_result);
	ASSERT_FALSE(*result.signature_result);
	expect_contains_errors(result.signature_result->errors,
		signature_verifier_errc::absent_signing_cert_issuer_and_sn);
	ASSERT_TRUE(std::holds_alternative<std::monostate>(result.signing_time));
	ASSERT_FALSE(result.cert_store);
}

TYPED_TEST(AuthenticodeTimestampPkcs7SignatureVerifierTest, InvalidMessageDigestData)
{
	this->init_algorithms();
	this->init_authenticated_attributes(this->invalid_message_digest_data);

	const auto result = this->verify();
	ASSERT_FALSE(result);
	expect_contains_errors(result.authenticode_format_errors,
		pkcs7::pkcs7_format_validator_errc::invalid_message_digest);
	expect_contains_errors(result.certificate_store_warnings);
	ASSERT_FALSE(result.hash_valid);
	ASSERT_FALSE(result.message_digest_valid);
	ASSERT_EQ(result.digest_alg, digest_algorithm::sha256);
	ASSERT_FALSE(result.imprint_digest_alg);
	ASSERT_EQ(result.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_FALSE(result.signature_result);
	this->check_signing_time(result);
	ASSERT_FALSE(result.cert_store);
}

TYPED_TEST(AuthenticodeTimestampPkcs7SignatureVerifierTest, AbsentSigningCert)
{
	this->init_algorithms();
	this->init_authenticated_attributes(this->valid_message_digest_data);

	const auto result = this->verify();
	ASSERT_FALSE(result);
	expect_contains_errors(result.authenticode_format_errors);
	expect_contains_errors(result.certificate_store_warnings);
	ASSERT_TRUE(result.hash_valid);
	ASSERT_FALSE(*result.hash_valid);
	ASSERT_FALSE(result.message_digest_valid);
	ASSERT_EQ(result.digest_alg, digest_algorithm::sha256);
	ASSERT_FALSE(result.imprint_digest_alg);
	ASSERT_EQ(result.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_TRUE(result.signature_result);
	ASSERT_FALSE(*result.signature_result);
	expect_contains_errors(result.signature_result->errors,
		signature_verifier_errc::absent_signing_cert_issuer_and_sn);
	this->check_signing_time(result);
	ASSERT_FALSE(result.cert_store);
}

TYPED_TEST(AuthenticodeTimestampPkcs7SignatureVerifierTest, Invalid1)
{
	this->init_algorithms();
	this->init_authenticated_attributes(this->valid_message_digest_data);
	this->init_signing_certificate();
	this->add_raw_authenticated_attributes(false);

	const auto result = this->verify();
	ASSERT_FALSE(result);
	expect_contains_errors(result.authenticode_format_errors);
	expect_contains_errors(result.certificate_store_warnings);
	ASSERT_TRUE(result.hash_valid);
	ASSERT_FALSE(*result.hash_valid);
	ASSERT_FALSE(result.message_digest_valid);
	ASSERT_EQ(result.digest_alg, digest_algorithm::sha256);
	ASSERT_FALSE(result.imprint_digest_alg);
	ASSERT_EQ(result.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_TRUE(result.signature_result);
	ASSERT_FALSE(*result.signature_result);
	expect_contains_errors(result.signature_result->errors);
	this->check_signing_time(result);
	ASSERT_FALSE(result.cert_store);
}

TYPED_TEST(AuthenticodeTimestampPkcs7SignatureVerifierTest, Invalid2)
{
	this->init_algorithms();
	this->init_authenticated_attributes(this->valid_message_digest_data);
	this->init_signing_certificate();
	this->add_raw_authenticated_attributes(true);

	const auto result = this->verify();
	ASSERT_FALSE(result);
	expect_contains_errors(result.authenticode_format_errors);
	expect_contains_errors(result.certificate_store_warnings);
	ASSERT_TRUE(result.hash_valid);
	ASSERT_FALSE(*result.hash_valid);
	ASSERT_FALSE(result.message_digest_valid);
	ASSERT_EQ(result.digest_alg, digest_algorithm::sha256);
	ASSERT_FALSE(result.imprint_digest_alg);
	ASSERT_EQ(result.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_TRUE(result.signature_result);
	ASSERT_TRUE(*result.signature_result);
	expect_contains_errors(result.signature_result->errors);
	this->check_signing_time(result);
	ASSERT_FALSE(result.cert_store);
}

TYPED_TEST(AuthenticodeTimestampPkcs7SignatureVerifierTest, Valid)
{
	this->init_algorithms();
	this->init_authenticated_attributes(this->valid_and_correct_message_digest_data);
	this->init_signing_certificate();
	this->add_raw_authenticated_attributes(true);

	auto result = this->verify();
	ASSERT_TRUE(result);
	expect_contains_errors(result.authenticode_format_errors);
	expect_contains_errors(result.certificate_store_warnings);
	ASSERT_TRUE(result.hash_valid);
	ASSERT_TRUE(*result.hash_valid);
	ASSERT_FALSE(result.message_digest_valid);
	ASSERT_EQ(result.digest_alg, digest_algorithm::sha256);
	ASSERT_FALSE(result.imprint_digest_alg);
	ASSERT_EQ(result.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_TRUE(result.signature_result);
	ASSERT_TRUE(*result.signature_result);
	expect_contains_errors(result.signature_result->errors);
	this->check_signing_time(result);
	ASSERT_FALSE(result.cert_store);

	result.signing_time = std::monostate{};
	ASSERT_FALSE(result);
}

namespace
{
template<typename T>
class AuthenticodeTimestampCmsSignatureVerifierTest
	: public AuthenticodeTimestampSignatureVerifierTestBase<typename T::range_type>
{
public:
	using signature_type = T;
	using range_type = typename signature_type::range_type;
	using base_type = AuthenticodeTimestampSignatureVerifierTestBase<range_type>;

public:
	auto verify() const
	{
		return verify_timestamp_signature(encrypted_digest, signature);
	}

	void add_signer_info_and_algorithm()
	{
		auto& signer_info = signature.get_content_info().data.signer_infos.emplace_back();
		signer_info.digest_algorithm.algorithm.container.assign(
			asn1::crypto::hash::id_sha256.begin(),
			asn1::crypto::hash::id_sha256.end());
		signer_info.digest_encryption_algorithm.algorithm.container.assign(
				asn1::crypto::pki::id_rsa.begin(),
				asn1::crypto::pki::id_rsa.end());

		auto& algorithm = signature.get_content_info().data.digest_algorithms.emplace_back();
		algorithm.algorithm.container.assign(
			asn1::crypto::hash::id_sha256.begin(),
			asn1::crypto::hash::id_sha256.end());

		signer_info.version = 1u;
	}

	void add_signed_data_version()
	{
		signature.get_content_info().data.version = 3u;
	}

	void add_tst_data_version()
	{
		signature.get_content_info().data.content_info.info.value.version = 1u;
	}

	void add_tst_data_oid()
	{
		signature.get_content_info().data.content_info.content_type.container.assign(
			asn1::crypto::pkcs9::oid_tst_info.begin(),
			asn1::crypto::pkcs9::oid_tst_info.end());
	}

	void add_signed_data_oid()
	{
		signature.get_content_info().content_type.container.assign(
			asn1::crypto::pkcs7::oid_signed_data.begin(),
			asn1::crypto::pkcs7::oid_signed_data.end());
	}

	void init_authenticated_attributes(const std::vector<range_type>& message_digest)
	{
		base_type::init_authenticated_attributes(
			signature.get_content_info().data.signer_infos
				.at(0).authenticated_attributes.emplace().value, message_digest);
	}

	void add_signing_time()
	{
		signature.get_content_info().data.content_info.info.value.gen_time = {
			.year = 96u,
			.month = 4u,
			.day = 15u,
			.hour = 20u,
			.minute = 30u,
			.second = 1u
		};
	}

	void add_imprint_algorithm(bool valid_hash)
	{
		auto& imprint = signature.get_content_info().data.content_info.info.value.imprint;
		imprint.hash_algorithm.algorithm.container.assign(
			asn1::crypto::hash::id_sha1.begin(),
			asn1::crypto::hash::id_sha1.end());
		imprint.hashed_message = valid_hash
			? valid_imprint_hash
			: invalid_imprint_hash;
	}

	void add_raw_signed_content()
	{
		signature.get_content_info().data.content_info.info.raw
			= valid_signed_content;
	}

	void set_issuer_and_sn()
	{
		auto& signer_info = signature.get_content_info().data.signer_infos.back();
		auto& issuer_and_sn = signer_info.sid.template emplace<0>();
		issuer_and_sn.issuer.raw = this->raw_issuer;
		issuer_and_sn.serial_number = this->serial_number;
	}

	void set_valid_raw_authenticated_attributes_and_encrypted_digest()
	{
		auto& signer_info = signature.get_content_info().data.signer_infos.back();
		signer_info.encrypted_digest = this->valid_rsa1_encrypted_digest;
		signer_info.authenticated_attributes->raw
			= this->valid_raw_authenticated_attributes;
	}

	void init_signing_certificate()
	{
		this->set_issuer_and_sn();
		auto& certificate = std::get<0>(
			signature.get_content_info().data.certificates.emplace().emplace_back());
		certificate.tbs_cert.issuer.raw = this->raw_issuer;
		certificate.tbs_cert.serial_number = this->serial_number;
		certificate.tbs_cert.pki.subject_publickey.container = this->valid_rsa1_public_key;
	}

	template <typename RangeType2>
	static void check_signing_time(const timestamp_signature_check_status<RangeType2>& result)
	{
		const auto* signing_time = std::get_if<asn1::generalized_time>(&result.signing_time);
		ASSERT_NE(signing_time, nullptr);
		ASSERT_EQ(signing_time->year, 96u);
		ASSERT_EQ(signing_time->month, 4u);
		ASSERT_EQ(signing_time->day, 15u);
		ASSERT_EQ(signing_time->hour, 20u);
		ASSERT_EQ(signing_time->minute, 30u);
		ASSERT_EQ(signing_time->second, 1u);
	}

	template<typename CertStore>
	void check_cert_store(const CertStore& cert_store) const
	{
		ASSERT_TRUE(cert_store);
		ASSERT_EQ(cert_store->size(), 1u);
		const auto* certificate = cert_store->find_certificate(
			this->serial_number, this->raw_issuer);
		ASSERT_NE(certificate, nullptr);
	}

public:
	static const inline std::vector encrypted_digest{ std::byte{'a'}, std::byte{'b'} };
	static const inline std::vector invalid_imprint_hash{ std::byte{'c'}, std::byte{'d'} };
	static const inline std::vector valid_imprint_hash{
		hex_string_to_bytes("da23614e02469a0d7c7bd1bdab5c9c474b1904dc") };

	signature_type signature{};

	static const inline std::vector valid_signed_content{
		// ASN.1 TAGGED and OCTET STRING placeholder
		std::byte{}, std::byte{}, std::byte{}, std::byte{},
		// data goes below
		std::byte{'a'}, std::byte{'b'}
	};
};

using cms_tested_range_types = ::testing::Types<
	authenticode_signature_cms_info_type<vector_range_type>,
	authenticode_signature_cms_info_type<span_range_type>,
	authenticode_signature_cms_info_ms_bug_workaround_type<vector_range_type>,
	authenticode_signature_cms_info_ms_bug_workaround_type<span_range_type>>;

} // namespace

TYPED_TEST_SUITE(AuthenticodeTimestampCmsSignatureVerifierTest, cms_tested_range_types);

TYPED_TEST(AuthenticodeTimestampCmsSignatureVerifierTest, Empty)
{
	const auto result = this->verify();
	ASSERT_FALSE(result);
	expect_contains_errors(result.authenticode_format_errors,
		pkcs7::pkcs7_format_validator_errc::invalid_signed_data_oid,
		pkcs7::pkcs7_format_validator_errc::invalid_signed_data_version,
		pkcs7::pkcs7_format_validator_errc::invalid_signer_count);
	expect_contains_errors(result.certificate_store_warnings);
	ASSERT_FALSE(result.hash_valid);
	ASSERT_FALSE(result.message_digest_valid);
	ASSERT_FALSE(result.digest_alg);
	ASSERT_FALSE(result.imprint_digest_alg);
	ASSERT_FALSE(result.digest_encryption_alg);
	ASSERT_FALSE(result.signature_result);
	ASSERT_TRUE(std::holds_alternative<std::monostate>(result.signing_time));
	ASSERT_FALSE(result.cert_store);
}

TYPED_TEST(AuthenticodeTimestampCmsSignatureVerifierTest, EmptyValidPkcs7)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();

	const auto result = this->verify();
	ASSERT_FALSE(result);
	expect_contains_errors(result.authenticode_format_errors,
		pkcs7::pkcs7_format_validator_errc::invalid_signer_count);
	expect_contains_errors(result.certificate_store_warnings);
	ASSERT_FALSE(result.hash_valid);
	ASSERT_FALSE(result.message_digest_valid);
	ASSERT_FALSE(result.digest_alg);
	ASSERT_FALSE(result.imprint_digest_alg);
	ASSERT_FALSE(result.digest_encryption_alg);
	ASSERT_FALSE(result.signature_result);
	ASSERT_TRUE(std::holds_alternative<std::monostate>(result.signing_time));
	ASSERT_FALSE(result.cert_store);
}

TYPED_TEST(AuthenticodeTimestampCmsSignatureVerifierTest, WithSignerInfo)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();

	const auto result = this->verify();
	ASSERT_FALSE(result);
	expect_contains_errors(result.authenticode_format_errors,
		authenticode_format_validator_errc::invalid_content_info_oid,
		authenticode_timestamp_signature_format_validator_errc::invalid_tst_info_version);
	expect_contains_errors(result.certificate_store_warnings);
	ASSERT_FALSE(result.hash_valid);
	ASSERT_FALSE(result.message_digest_valid);
	ASSERT_FALSE(result.digest_alg);
	ASSERT_FALSE(result.imprint_digest_alg);
	ASSERT_FALSE(result.digest_encryption_alg);
	ASSERT_FALSE(result.signature_result);
	ASSERT_TRUE(std::holds_alternative<std::monostate>(result.signing_time));
	ASSERT_FALSE(result.cert_store);
}

TYPED_TEST(AuthenticodeTimestampCmsSignatureVerifierTest, WithContentInfo)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_tst_data_oid();
	this->add_tst_data_version();

	const auto result = this->verify();
	ASSERT_FALSE(result);
	expect_contains_errors(result.authenticode_format_errors,
		pkcs7::pkcs7_format_validator_errc::absent_message_digest,
		pkcs7::pkcs7_format_validator_errc::absent_content_type);
	expect_contains_errors(result.certificate_store_warnings);
	ASSERT_FALSE(result.hash_valid);
	ASSERT_FALSE(result.message_digest_valid);
	ASSERT_FALSE(result.digest_alg);
	ASSERT_FALSE(result.imprint_digest_alg);
	ASSERT_FALSE(result.digest_encryption_alg);
	ASSERT_FALSE(result.signature_result);
	ASSERT_TRUE(std::holds_alternative<std::monostate>(result.signing_time));
	ASSERT_FALSE(result.cert_store);
}

TYPED_TEST(AuthenticodeTimestampCmsSignatureVerifierTest, WithAuthenticatedAttributes)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_tst_data_oid();
	this->add_tst_data_version();
	this->add_signing_time();
	this->init_authenticated_attributes(this->invalid_message_digest_data);

	const auto result = this->verify();
	ASSERT_FALSE(result);
	expect_contains_errors(result.authenticode_format_errors,
		crypto_algorithm_errc::unsupported_digest_algorithm);
	expect_contains_errors(result.certificate_store_warnings,
		certificate_store_errc::absent_certificates);
	ASSERT_FALSE(result.hash_valid);
	ASSERT_FALSE(result.message_digest_valid);
	ASSERT_EQ(result.digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.imprint_digest_alg, digest_algorithm::unknown);
	ASSERT_EQ(result.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_FALSE(result.signature_result);
	this->check_signing_time(result);
	ASSERT_TRUE(result.cert_store);
	ASSERT_TRUE(result.cert_store->empty());
}

TYPED_TEST(AuthenticodeTimestampCmsSignatureVerifierTest, WithImprintAlgorithm)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_tst_data_oid();
	this->add_tst_data_version();
	this->add_signing_time();
	this->add_imprint_algorithm(false);
	this->init_authenticated_attributes(this->invalid_message_digest_data);

	const auto result = this->verify();
	ASSERT_FALSE(result);
	expect_contains_errors(result.authenticode_format_errors,
		pkcs7::pkcs7_format_validator_errc::invalid_message_digest);
	expect_contains_errors(result.certificate_store_warnings,
		certificate_store_errc::absent_certificates);
	ASSERT_FALSE(result.message_digest_valid);
	ASSERT_TRUE(result.hash_valid);
	ASSERT_FALSE(*result.hash_valid);
	ASSERT_EQ(result.digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.imprint_digest_alg, digest_algorithm::sha1);
	ASSERT_EQ(result.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_FALSE(result.signature_result);
	this->check_signing_time(result);
	ASSERT_TRUE(result.cert_store);
	ASSERT_TRUE(result.cert_store->empty());
}

TYPED_TEST(AuthenticodeTimestampCmsSignatureVerifierTest, WithValidMessageDigest)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_tst_data_oid();
	this->add_tst_data_version();
	this->add_signing_time();
	this->add_imprint_algorithm(false);
	this->add_raw_signed_content();
	this->init_authenticated_attributes(this->valid_message_digest_data);

	const auto result = this->verify();
	ASSERT_FALSE(result);
	expect_contains_errors(result.authenticode_format_errors);
	expect_contains_errors(result.certificate_store_warnings,
		certificate_store_errc::absent_certificates);
	ASSERT_TRUE(result.message_digest_valid);
	ASSERT_FALSE(*result.message_digest_valid);
	ASSERT_TRUE(result.hash_valid);
	ASSERT_FALSE(*result.hash_valid);
	ASSERT_EQ(result.digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.imprint_digest_alg, digest_algorithm::sha1);
	ASSERT_EQ(result.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_TRUE(result.signature_result);
	ASSERT_FALSE(*result.signature_result);
	this->check_signing_time(result);
	ASSERT_TRUE(result.cert_store);
	ASSERT_TRUE(result.cert_store->empty());
}

TYPED_TEST(AuthenticodeTimestampCmsSignatureVerifierTest, WithValidImprintHash)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_tst_data_oid();
	this->add_tst_data_version();
	this->add_signing_time();
	this->add_imprint_algorithm(true);
	this->add_raw_signed_content();
	this->init_authenticated_attributes(this->valid_message_digest_data);

	const auto result = this->verify();
	ASSERT_FALSE(result);
	expect_contains_errors(result.authenticode_format_errors);
	expect_contains_errors(result.certificate_store_warnings,
		certificate_store_errc::absent_certificates);
	ASSERT_TRUE(result.message_digest_valid);
	ASSERT_FALSE(*result.message_digest_valid);
	ASSERT_TRUE(result.hash_valid);
	ASSERT_TRUE(*result.hash_valid);
	ASSERT_EQ(result.digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.imprint_digest_alg, digest_algorithm::sha1);
	ASSERT_EQ(result.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_TRUE(result.signature_result);
	ASSERT_FALSE(*result.signature_result);
	this->check_signing_time(result);
	ASSERT_TRUE(result.cert_store);
	ASSERT_TRUE(result.cert_store->empty());
}

TYPED_TEST(AuthenticodeTimestampCmsSignatureVerifierTest, WithCorrectMessageDigest)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_tst_data_oid();
	this->add_tst_data_version();
	this->add_signing_time();
	this->add_imprint_algorithm(true);
	this->add_raw_signed_content();
	this->init_authenticated_attributes(this->valid_and_correct_message_digest_data);

	const auto result = this->verify();
	ASSERT_FALSE(result);
	expect_contains_errors(result.authenticode_format_errors);
	expect_contains_errors(result.certificate_store_warnings,
		certificate_store_errc::absent_certificates);
	ASSERT_TRUE(result.message_digest_valid);
	ASSERT_TRUE(*result.message_digest_valid);
	ASSERT_TRUE(result.hash_valid);
	ASSERT_TRUE(*result.hash_valid);
	ASSERT_EQ(result.digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.imprint_digest_alg, digest_algorithm::sha1);
	ASSERT_EQ(result.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_TRUE(result.signature_result);
	ASSERT_FALSE(*result.signature_result);
	this->check_signing_time(result);
	ASSERT_TRUE(result.cert_store);
	ASSERT_TRUE(result.cert_store->empty());
}

TYPED_TEST(AuthenticodeTimestampCmsSignatureVerifierTest, WithCertStore)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_tst_data_oid();
	this->add_tst_data_version();
	this->add_signing_time();
	this->add_imprint_algorithm(true);
	this->add_raw_signed_content();
	this->init_signing_certificate();
	this->init_authenticated_attributes(this->valid_and_correct_message_digest_data);

	const auto result = this->verify();
	ASSERT_FALSE(result);
	expect_contains_errors(result.authenticode_format_errors);
	expect_contains_errors(result.certificate_store_warnings);
	ASSERT_TRUE(result.message_digest_valid);
	ASSERT_TRUE(*result.message_digest_valid);
	ASSERT_TRUE(result.hash_valid);
	ASSERT_TRUE(*result.hash_valid);
	ASSERT_EQ(result.digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.imprint_digest_alg, digest_algorithm::sha1);
	ASSERT_EQ(result.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_TRUE(result.signature_result);
	ASSERT_FALSE(*result.signature_result);
	this->check_signing_time(result);
	this->check_cert_store(result.cert_store);
}

TYPED_TEST(AuthenticodeTimestampCmsSignatureVerifierTest, Valid)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_tst_data_oid();
	this->add_tst_data_version();
	this->add_signing_time();
	this->add_imprint_algorithm(true);
	this->add_raw_signed_content();
	this->init_signing_certificate();
	this->init_authenticated_attributes(this->valid_and_correct_message_digest_data);
	this->set_valid_raw_authenticated_attributes_and_encrypted_digest();

	const auto result = this->verify();
	ASSERT_TRUE(result);
	expect_contains_errors(result.authenticode_format_errors);
	expect_contains_errors(result.certificate_store_warnings);
	ASSERT_TRUE(result.message_digest_valid);
	ASSERT_TRUE(*result.message_digest_valid);
	ASSERT_TRUE(result.hash_valid);
	ASSERT_TRUE(*result.hash_valid);
	ASSERT_EQ(result.digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.imprint_digest_alg, digest_algorithm::sha1);
	ASSERT_EQ(result.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_TRUE(result.signature_result);
	ASSERT_TRUE(*result.signature_result);
	this->check_signing_time(result);
	this->check_cert_store(result.cert_store);
}
