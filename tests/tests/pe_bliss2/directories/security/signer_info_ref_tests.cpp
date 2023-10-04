#include "pe_bliss2/security/pkcs7/signer_info_ref.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <type_traits>
#include <vector>

#include "gtest/gtest.h"

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/byte_range_types.h"

#include "simple_asn1/crypto/algorithms.h"
#include "simple_asn1/crypto/pkcs7/oids.h"

using namespace pe_bliss::security;
using namespace pe_bliss::security::pkcs7;

namespace
{
template<template<typename> typename SignerInfoRef, typename RangeType>
struct signer_info_ref_types
{
	using range_type = RangeType;
	using type = SignerInfoRef<range_type>;
};

template<typename T>
class SignerInfoRefPkcs7Test : public testing::Test
{
public:
	using range_type = typename T::range_type;
	using signer_info_ref_type = typename T::type;
	static constexpr bool is_cms = std::is_same_v<signer_info_ref_type,
		signer_info_ref_cms<range_type>>;

public:
	template<typename AttributeMapGetter, typename RawAttributes>
	static void test_attributes(const AttributeMapGetter& map_getter,
		RawAttributes& raw_attributes)
	{
		ASSERT_TRUE(map_getter().empty());
		raw_attributes = {
			asn1::crypto::pkcs7::attribute<range_type>{
				.type = asn1::crypto::object_identifier_type{
					.container = { 1, 2, 3 }
				}
			},
			asn1::crypto::pkcs7::attribute<range_type>{
				.type = asn1::crypto::object_identifier_type{
					.container = { 1, 2, 3, 4, 5 }
				}
			}
		};

		auto map = map_getter();
		ASSERT_EQ(map.size(), 2u);
		auto it = map.find(std::vector<std::uint32_t>{1, 2, 3});
		ASSERT_NE(it, map.end());
		ASSERT_EQ(&it->second.get(), &raw_attributes[0].values);
	}

	template<typename AttributeMapGetter, typename RawAttributes>
	static void test_attributes_duplicate(const AttributeMapGetter& map_getter,
		RawAttributes& raw_attributes)
	{
		ASSERT_TRUE(map_getter().empty());
		raw_attributes = {
			asn1::crypto::pkcs7::attribute<range_type>{
				.type = asn1::crypto::object_identifier_type{
					.container = { 1, 2, 3 }
				}
			},
			asn1::crypto::pkcs7::attribute<range_type>{
				.type = asn1::crypto::object_identifier_type{
					.container = { 1, 2, 3 }
				}
			}
		};

		ASSERT_THROW((void)map_getter(), pe_bliss::pe_error);
	}

public:
	typename signer_info_ref_type::signer_info_type signer_info{};
	signer_info_ref_type signer_info_ref{ signer_info };
};

using tested_signer_info_ref_types = ::testing::Types<
	signer_info_ref_types<signer_info_ref_pkcs7, vector_range_type>,
	signer_info_ref_types<signer_info_ref_pkcs7, span_range_type>,
	signer_info_ref_types<signer_info_ref_cms, vector_range_type>,
	signer_info_ref_types<signer_info_ref_cms, span_range_type>>;
} //namespace

TYPED_TEST_SUITE(SignerInfoRefPkcs7Test, tested_signer_info_ref_types);

TYPED_TEST(SignerInfoRefPkcs7Test, GetDigestAlgorithm)
{
	ASSERT_EQ(this->signer_info_ref.get_digest_algorithm(), digest_algorithm::unknown);

	const std::vector<std::uint32_t> md5_alg(asn1::crypto::hash::id_md5.cbegin(),
		asn1::crypto::hash::id_md5.cend());
	this->signer_info.digest_algorithm.algorithm.container = md5_alg;

	ASSERT_EQ(this->signer_info_ref.get_digest_algorithm(), digest_algorithm::md5);
}

TYPED_TEST(SignerInfoRefPkcs7Test, GetDigestEncryptionAlgorithm)
{
	ASSERT_EQ(this->signer_info_ref.get_digest_encryption_algorithm(),
		encryption_and_hash_algorithm{});

	const std::vector<std::uint32_t> md5_with_rsa_alg(asn1::crypto::pki::id_md5_with_rsa.cbegin(),
		asn1::crypto::pki::id_md5_with_rsa.cend());
	this->signer_info.digest_encryption_algorithm.algorithm.container = md5_with_rsa_alg;

	ASSERT_EQ(this->signer_info_ref.get_digest_encryption_algorithm(),
		(encryption_and_hash_algorithm{
			.encryption_alg = digest_encryption_algorithm::rsa,
			.hash_alg = digest_algorithm::md5 }));
}

TYPED_TEST(SignerInfoRefPkcs7Test, GetAuthenticatedAttributes)
{
	this->test_attributes([this]() {
		return this->signer_info_ref.get_authenticated_attributes().get_map(); },
		this->signer_info.authenticated_attributes.emplace().value);
}

TYPED_TEST(SignerInfoRefPkcs7Test, GetUnauthenticatedAttributes)
{
	this->test_attributes([this]() {
		return this->signer_info_ref.get_unauthenticated_attributes().get_map(); },
		this->signer_info.unauthenticated_attributes.emplace());
}

TYPED_TEST(SignerInfoRefPkcs7Test, GetAuthenticatedAttributesDuplicate)
{
	this->test_attributes_duplicate([this]() {
		return this->signer_info_ref.get_authenticated_attributes().get_map(); },
		this->signer_info.authenticated_attributes.emplace().value);
}

TYPED_TEST(SignerInfoRefPkcs7Test, GetUnauthenticatedAttributesDuplicate)
{
	this->test_attributes_duplicate([this]() {
		return this->signer_info_ref.get_unauthenticated_attributes().get_map(); },
		this->signer_info.unauthenticated_attributes.emplace());
}

TYPED_TEST(SignerInfoRefPkcs7Test, GetUnderlying)
{
	ASSERT_EQ(&this->signer_info_ref.get_underlying(), &this->signer_info);
}

TYPED_TEST(SignerInfoRefPkcs7Test, GetEncryptedDigest)
{
	ASSERT_TRUE(this->signer_info_ref.get_encrypted_digest().empty());
	const std::vector<std::byte> vec{ std::byte{1}, std::byte{2} };
	this->signer_info.encrypted_digest = vec;
	ASSERT_TRUE(std::ranges::equal(vec, this->signer_info_ref.get_encrypted_digest()));
}

TYPED_TEST(SignerInfoRefPkcs7Test, CalculateMessageDigest)
{
	const std::vector<std::byte> raw_signed_content1{ std::byte{1}, std::byte{2}, std::byte{3} };
	const std::vector<std::byte> raw_signed_content2{ std::byte{4}, std::byte{5} };

	//No hash algorithm specified - throws
	ASSERT_THROW((void)this->signer_info_ref.calculate_message_digest(
		std::array<const span_range_type, 2>{ raw_signed_content1, raw_signed_content2 }),
		pe_bliss::pe_error);

	const std::vector<std::uint32_t> md5_alg(asn1::crypto::hash::id_md5.cbegin(),
		asn1::crypto::hash::id_md5.cend());
	this->signer_info.digest_algorithm.algorithm.container = md5_alg;
	const std::vector<std::byte> md5_hash{ std::byte{0x7c}, std::byte{0xfd},
		std::byte{0xd0}, std::byte{0x78}, std::byte{0x89}, std::byte{0xb3},
		std::byte{0x29}, std::byte{0x5d}, std::byte{0x6a}, std::byte{0x55},
		std::byte{0x09}, std::byte{0x14}, std::byte{0xab}, std::byte{0x35},
		std::byte{0xe0}, std::byte{0x68} };
	ASSERT_EQ(this->signer_info_ref.calculate_message_digest(
		std::array<const span_range_type, 2>{ raw_signed_content1, raw_signed_content2 }), md5_hash);
}

TYPED_TEST(SignerInfoRefPkcs7Test, CalculateAuthenticatedAttributesDigestNoAttrs)
{
	// No authenticated attributes - throws
	ASSERT_THROW((void)this->signer_info_ref.calculate_authenticated_attributes_digest(),
		pe_bliss::pe_error);
}

TYPED_TEST(SignerInfoRefPkcs7Test, CalculateAuthenticatedAttributesDigest)
{
	const std::vector<std::byte> raw_attr_data{
		std::byte{0xff}, //this byte will be replaced with ASN.1 SET_OF tag (0x31)
		std::byte{1}, std::byte{2}, std::byte{3}
	};
	this->signer_info.authenticated_attributes.emplace().raw = raw_attr_data;
	const std::vector<std::uint32_t> md5_alg(asn1::crypto::hash::id_md5.cbegin(),
		asn1::crypto::hash::id_md5.cend());
	this->signer_info.digest_algorithm.algorithm.container = md5_alg;

	const std::vector<std::byte> md5_hash{ std::byte{0x37}, std::byte{0x1f},
		std::byte{0x7d}, std::byte{0xea}, std::byte{0xb6}, std::byte{0xa2}, std::byte{0x69},
		std::byte{0x82}, std::byte{0x0e}, std::byte{0xa2}, std::byte{0x1c}, std::byte{0xb3},
		std::byte{0x64}, std::byte{0xc0}, std::byte{0xd0}, std::byte{0x95} };
	ASSERT_EQ(this->signer_info_ref.calculate_authenticated_attributes_digest(), md5_hash);
}

TYPED_TEST(SignerInfoRefPkcs7Test, GetSignerCertificateIssuerAndSerialNumber)
{
	auto result = this->signer_info_ref
		.get_signer_certificate_issuer_and_serial_number();

	if constexpr (TestFixture::is_cms)
	{
		ASSERT_EQ(result.issuer, &std::get<asn1::crypto::pkcs7::issuer_and_serial_number<
			typename TestFixture::range_type>>(this->signer_info.sid).issuer.raw);
		ASSERT_EQ(result.serial_number, &std::get<asn1::crypto::pkcs7::issuer_and_serial_number<
			typename TestFixture::range_type>>(this->signer_info.sid).serial_number);

		this->signer_info.sid.emplace<1>();
		result = this->signer_info_ref
			.get_signer_certificate_issuer_and_serial_number();
		ASSERT_EQ(result.issuer, nullptr);
		ASSERT_EQ(result.serial_number, nullptr);
	}
	else
	{
		ASSERT_EQ(result.issuer, &this->signer_info.issuer_and_sn.issuer.raw);
		ASSERT_EQ(result.serial_number, &this->signer_info.issuer_and_sn.serial_number);
	}
}
