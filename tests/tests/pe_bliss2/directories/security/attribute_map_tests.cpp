#include "pe_bliss2/security/pkcs7/attribute_map.h"

#include <algorithm>
#include <cstddef>
#include <vector>

#include "gtest/gtest.h"

#include "pe_bliss2/pe_error.h"

#include "simple_asn1/crypto/algorithms.h"
#include "simple_asn1/crypto/pkcs7/oids.h"
#include "simple_asn1/decode.h"

using namespace pe_bliss::security;
using namespace pe_bliss::security::pkcs7;

namespace
{
template<typename T>
class AttributeMapTests : public testing::Test
{
public:
	using range_type = T;
	using map_type = attribute_map<range_type>;

public:
	const std::vector<std::uint32_t> test_attr{ 1, 2, 3 };
	const std::vector<std::uint32_t> test_attr2{ 1, 2, 3, 4, 5 };
	const std::vector<std::byte> test_data1{ std::byte{1}, std::byte{2} };
	const std::vector<std::byte> test_data2{ std::byte{5}, std::byte{7} };
	const std::vector<range_type> test_single_data{ test_data1 };
	const std::vector<range_type> test_multiple_data{ test_data1, test_data2 };
};

using tested_range_types = ::testing::Types<
	vector_range_type, span_range_type>;
} //namespace

TYPED_TEST_SUITE(AttributeMapTests, tested_range_types);

TYPED_TEST(AttributeMapTests, GetAttribute)
{
	typename TestFixture::map_type map{};
	ASSERT_FALSE(map.get_attribute(TestFixture::test_attr));
	map.get_map().try_emplace(TestFixture::test_attr, TestFixture::test_single_data);

	ASSERT_FALSE(map.get_attribute(TestFixture::test_attr2));
	auto result = map.get_attribute(TestFixture::test_attr);
	ASSERT_TRUE(result);
	ASSERT_TRUE(std::ranges::equal(*result, TestFixture::test_data1));
}

TYPED_TEST(AttributeMapTests, GetAttributeAbsentData)
{
	typename TestFixture::map_type map{};
	const std::vector<typename TestFixture::range_type> empty_vec;
	map.get_map().try_emplace(TestFixture::test_attr, empty_vec);

	ASSERT_THROW((void)map.get_attribute(TestFixture::test_attr), pe_bliss::pe_error);
}

TYPED_TEST(AttributeMapTests, GetAttributeMultipleData)
{
	typename TestFixture::map_type map{};
	map.get_map().try_emplace(TestFixture::test_attr, TestFixture::test_multiple_data);

	ASSERT_THROW((void)map.get_attribute(TestFixture::test_attr), pe_bliss::pe_error);
}

TYPED_TEST(AttributeMapTests, GetAttributes)
{
	typename TestFixture::map_type map{};
	ASSERT_TRUE(map.get_attributes(TestFixture::test_attr).empty());
	map.get_map().try_emplace(TestFixture::test_attr, TestFixture::test_single_data);

	ASSERT_TRUE(map.get_attributes(TestFixture::test_attr2).empty());
	auto result = map.get_attributes(TestFixture::test_attr);
	ASSERT_EQ(result.size(), 1u);
	ASSERT_TRUE(std::ranges::equal(result[0], TestFixture::test_data1));
}

TYPED_TEST(AttributeMapTests, GetAttributesAbsentData)
{
	typename TestFixture::map_type map{};
	const std::vector<typename TestFixture::range_type> empty_vec;
	map.get_map().try_emplace(TestFixture::test_attr, empty_vec);

	ASSERT_THROW((void)map.get_attributes(TestFixture::test_attr), pe_bliss::pe_error);
}

TYPED_TEST(AttributeMapTests, GetAttributesMultipleData)
{
	typename TestFixture::map_type map{};
	map.get_map().try_emplace(TestFixture::test_attr, TestFixture::test_multiple_data);

	auto result = map.get_attributes(TestFixture::test_attr);
	ASSERT_EQ(result.size(), 2u);
	ASSERT_TRUE(std::ranges::equal(result[0], TestFixture::test_data1));
	ASSERT_TRUE(std::ranges::equal(result[1], TestFixture::test_data2));
}

TYPED_TEST(AttributeMapTests, GetSpecificAttribute)
{
	typename TestFixture::map_type map{};
	ASSERT_FALSE(map.get_message_digest());
	ASSERT_FALSE(map.get_content_type());
	ASSERT_FALSE(map.get_signing_time());
	const std::vector<std::uint32_t> oid_message_digest(
		asn1::crypto::pkcs7::oid_message_digest.cbegin(),
		asn1::crypto::pkcs7::oid_message_digest.cend());
	map.get_map().try_emplace(oid_message_digest, TestFixture::test_single_data);
	{
		auto result = map.get_message_digest();
		ASSERT_TRUE(result);
		ASSERT_TRUE(std::ranges::equal(*result, TestFixture::test_data1));
	}

	map.get_map().clear();
	const std::vector<std::uint32_t> oid_content_type(
		asn1::crypto::pkcs7::oid_content_type.cbegin(),
		asn1::crypto::pkcs7::oid_content_type.cend());
	map.get_map().try_emplace(oid_content_type, TestFixture::test_single_data);
	{
		auto result = map.get_content_type();
		ASSERT_TRUE(result);
		ASSERT_TRUE(std::ranges::equal(*result, TestFixture::test_data1));
	}

	map.get_map().clear();
	const std::vector<std::uint32_t> oid_signing_time(
		asn1::crypto::pkcs7::oid_signing_time.cbegin(),
		asn1::crypto::pkcs7::oid_signing_time.cend());
	map.get_map().try_emplace(oid_signing_time, TestFixture::test_single_data);
	{
		auto result = map.get_signing_time();
		ASSERT_TRUE(result);
		ASSERT_TRUE(std::ranges::equal(*result, TestFixture::test_data1));
	}
}

TYPED_TEST(AttributeMapTests, VerifyMessageDigestAbsentAttribute)
{
	typename TestFixture::map_type map{};
	ASSERT_FALSE(verify_message_digest_attribute({}, map));
}

TYPED_TEST(AttributeMapTests, VerifyMessageDigestInvalidAttribute)
{
	typename TestFixture::map_type map{};
	const std::vector<std::uint32_t> oid_message_digest(
		asn1::crypto::pkcs7::oid_message_digest.cbegin(),
		asn1::crypto::pkcs7::oid_message_digest.cend());
	// Invalid ASN.1 OCTET STRING
	map.get_map().try_emplace(oid_message_digest, TestFixture::test_single_data);

	ASSERT_THROW((void)verify_message_digest_attribute({}, map), asn1::parse_error);
}

TYPED_TEST(AttributeMapTests, VerifyMessageDigestAttribute)
{
	typename TestFixture::map_type map{};
	const std::vector<std::uint32_t> oid_message_digest(
		asn1::crypto::pkcs7::oid_message_digest.cbegin(),
		asn1::crypto::pkcs7::oid_message_digest.cend());
	const std::vector<std::byte> message_digest{
		std::byte{4}, //ASN.1 OCTET STRING type
		std::byte{2}, //length
		std::byte{5}, std::byte{15} /* value */ };
	const std::vector<typename TestFixture::range_type> single_message_digest{ message_digest };
	map.get_map().try_emplace(oid_message_digest, single_message_digest);

	ASSERT_FALSE(verify_message_digest_attribute({}, map));
	ASSERT_TRUE(verify_message_digest_attribute(std::span(message_digest).subspan(2), map));
}
