#include "pe_bliss2/security/authenticode_page_hashes.h"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/byte_range_types.h"

#include "simple_asn1/crypto/pkcs7/authenticode/oids.h"

using namespace pe_bliss::security;

namespace
{
template<typename Ranges>
class AuthenticodePageHashesTests : public testing::Test
{
public:
	using target_range_type = typename Ranges::first_type;
	using source_range_type = typename Ranges::second_type;
	using hashes_type = authenticode_page_hashes<typename Ranges::first_type>;
	using authenticode_type = authenticode_pkcs7<typename Ranges::second_type>;

public:
	hashes_type hashes;
	authenticode_type authenticode;
};

using tested_range_types = ::testing::Types<
	std::pair<vector_range_type, span_range_type>,
	std::pair<vector_range_type, vector_range_type>,
	std::pair<span_range_type, vector_range_type>,
	std::pair<span_range_type, span_range_type>>;
} //namespace

TYPED_TEST_SUITE(AuthenticodePageHashesTests, tested_range_types);

TYPED_TEST(AuthenticodePageHashesTests, GetRawPageHashesEmpty)
{
	ASSERT_FALSE(this->hashes.get_raw_page_hashes());
}

TYPED_TEST(AuthenticodePageHashesTests, GetRawPageHashesInvalid1)
{
	this->hashes.get_underlying_struct().resize(2u);
	ASSERT_FALSE(this->hashes.get_raw_page_hashes());
}

TYPED_TEST(AuthenticodePageHashesTests, GetRawPageHashesInvalid2)
{
	this->hashes.get_underlying_struct().emplace_back().hashes.resize(2);
	ASSERT_FALSE(this->hashes.get_raw_page_hashes());
}

TYPED_TEST(AuthenticodePageHashesTests, GetRawPageHashesValid)
{
	const std::vector<std::byte> value{ std::byte{1}, std::byte{2} };
	this->hashes.get_underlying_struct().emplace_back()
		.hashes.emplace_back(value);
	auto result = this->hashes.get_raw_page_hashes();
	ASSERT_TRUE(result);
	ASSERT_TRUE(std::ranges::equal(*result, value));
}

TYPED_TEST(AuthenticodePageHashesTests, IsValidEmpty)
{
	ASSERT_FALSE(this->hashes.is_valid(digest_algorithm::md5));
}

TYPED_TEST(AuthenticodePageHashesTests, IsValidInvalidOid)
{
	const std::vector<std::byte> value(20);
	auto& hash = this->hashes.get_underlying_struct().emplace_back();
	hash.hashes.emplace_back(value);
	hash.type.container = {
		asn1::crypto::pkcs7::authenticode::oid_spc_pe_image_data.cbegin(),
		asn1::crypto::pkcs7::authenticode::oid_spc_pe_image_data.cend() };
	ASSERT_FALSE(this->hashes.is_valid(digest_algorithm::md5));
}

TYPED_TEST(AuthenticodePageHashesTests, IsValidFullyValid)
{
	const std::vector<std::byte> value(20);
	auto& hash = this->hashes.get_underlying_struct().emplace_back();
	hash.hashes.emplace_back(value);

	hash.type.container = {
		asn1::crypto::pkcs7::authenticode::oid_spc_page_hash_v1.cbegin(),
		asn1::crypto::pkcs7::authenticode::oid_spc_page_hash_v1.cend() };
	ASSERT_TRUE(this->hashes.is_valid(digest_algorithm::md5));

	hash.type.container = {
		asn1::crypto::pkcs7::authenticode::oid_spc_page_hash_v2.cbegin(),
		asn1::crypto::pkcs7::authenticode::oid_spc_page_hash_v2.cend() };
	ASSERT_TRUE(this->hashes.is_valid(digest_algorithm::md5));
}

TYPED_TEST(AuthenticodePageHashesTests, IsValidInvalidHashSize)
{
	const std::vector<std::byte> value(21);
	auto& hash = this->hashes.get_underlying_struct().emplace_back();
	hash.hashes.emplace_back(value);

	hash.type.container = {
		asn1::crypto::pkcs7::authenticode::oid_spc_page_hash_v1.cbegin(),
		asn1::crypto::pkcs7::authenticode::oid_spc_page_hash_v1.cend() };
	ASSERT_FALSE(this->hashes.is_valid(digest_algorithm::md5));
}

TYPED_TEST(AuthenticodePageHashesTests, IsValidInvalidHash)
{
	const std::vector<std::byte> value(20);
	auto& hash = this->hashes.get_underlying_struct().emplace_back();
	hash.hashes.emplace_back(value);

	hash.type.container = {
		asn1::crypto::pkcs7::authenticode::oid_spc_page_hash_v1.cbegin(),
		asn1::crypto::pkcs7::authenticode::oid_spc_page_hash_v1.cend() };
	ASSERT_FALSE(this->hashes.is_valid(digest_algorithm::unknown));
}

TYPED_TEST(AuthenticodePageHashesTests, GetPageHashesEmpty)
{
	ASSERT_FALSE(get_page_hashes<typename TestFixture::target_range_type>(this->authenticode));
}

TYPED_TEST(AuthenticodePageHashesTests, GetPageHashesNoSerializedObject)
{
	auto& file = this->authenticode.get_content_info()
		.data.content_info.content.type_value.value.value.file.emplace();
	file = std::string("test");
	ASSERT_FALSE(get_page_hashes<typename TestFixture::target_range_type>(this->authenticode));
}

TYPED_TEST(AuthenticodePageHashesTests, GetPageHashesSerializedObjectOidMismatch)
{
	auto& file = this->authenticode.get_content_info()
		.data.content_info.content.type_value.value.value.file.emplace();
	auto& obj = file.emplace<asn1::crypto::pkcs7::authenticode::spc_serialized_object<
		typename TestFixture::source_range_type>>();
	const std::vector<std::byte> value{ std::byte{1}, std::byte{2} };
	obj.class_id = value;

	ASSERT_FALSE(get_page_hashes<typename TestFixture::target_range_type>(this->authenticode));
}

TYPED_TEST(AuthenticodePageHashesTests, GetPageHashesSerializedObjectInvalidAsn1)
{
	auto& file = this->authenticode.get_content_info()
		.data.content_info.content.type_value.value.value.file.emplace();
	auto& obj = file.emplace<asn1::crypto::pkcs7::authenticode::spc_serialized_object<
		typename TestFixture::source_range_type>>();
	const std::vector<std::byte> value{
		asn1::crypto::pkcs7::authenticode::page_hashes_class_id.cbegin(),
		asn1::crypto::pkcs7::authenticode::page_hashes_class_id.cend() };
	obj.class_id = value;

	ASSERT_THROW((void)get_page_hashes<typename TestFixture::target_range_type>(this->authenticode),
		pe_bliss::pe_error);
}

TYPED_TEST(AuthenticodePageHashesTests, GetPageHashesSerializedObjectValid)
{
	auto& file = this->authenticode.get_content_info()
		.data.content_info.content.type_value.value.value.file.emplace();
	auto& obj = file.emplace<asn1::crypto::pkcs7::authenticode::spc_serialized_object<
		typename TestFixture::source_range_type>>();
	const std::vector<std::byte> value{
		asn1::crypto::pkcs7::authenticode::page_hashes_class_id.cbegin(),
		asn1::crypto::pkcs7::authenticode::page_hashes_class_id.cend() };
	obj.class_id = value;

	const std::vector<std::byte> asn1_data{
		std::byte{ 0x31u }, //SET OF
		std::byte{13}, //set of length

		std::byte{ 0x30u }, //SEQUENCE
		std::byte{11}, //sequence length

		std::byte{0x06u}, //OBJECT IDENTIFIER
		std::byte{2}, //object identifier length
		std::byte{0x2au}, std::byte{0x03u}, //1.2.3

		std::byte{ 0x31u }, //SET OF
		std::byte{5}, //set of length

		std::byte{ 0x04u }, //OCTET STRING
		std::byte{3}, //octet string length
		std::byte{4}, std::byte{5}, std::byte{6},

		std::byte{}, std::byte{} //some trailing null bytes
	};
	obj.serialized_data = asn1_data;

	auto result = get_page_hashes<typename TestFixture::target_range_type>(this->authenticode);
	ASSERT_TRUE(result);
	ASSERT_EQ(result->get_underlying_struct().size(), 1u);
	ASSERT_EQ(result->get_underlying_struct()[0].hashes.size(), 1u);
	ASSERT_EQ(result->get_underlying_struct()[0].type.container,
		(std::vector<std::uint32_t>{1, 2, 3}));
	ASSERT_TRUE(std::ranges::equal(result->get_underlying_struct()[0].hashes[0],
		(std::vector{ std::byte{4}, std::byte{5}, std::byte{6} })));
}

TYPED_TEST(AuthenticodePageHashesTests, GetPageHashesSerializedObjectAsn1TrailingGarbage)
{
	auto& file = this->authenticode.get_content_info()
		.data.content_info.content.type_value.value.value.file.emplace();
	auto& obj = file.emplace<asn1::crypto::pkcs7::authenticode::spc_serialized_object<
		typename TestFixture::source_range_type>>();
	const std::vector<std::byte> value{
		asn1::crypto::pkcs7::authenticode::page_hashes_class_id.cbegin(),
		asn1::crypto::pkcs7::authenticode::page_hashes_class_id.cend() };
	obj.class_id = value;

	const std::vector<std::byte> asn1_data{
		std::byte{ 0x31u }, //SET OF
		std::byte{13}, //set of length

		std::byte{ 0x30u }, //SEQUENCE
		std::byte{11}, //sequence length

		std::byte{0x06u}, //OBJECT IDENTIFIER
		std::byte{2}, //object identifier length
		std::byte{0x2au}, std::byte{0x03u}, //1.2.3

		std::byte{ 0x31u }, //SET OF
		std::byte{5}, //set of length

		std::byte{ 0x04u }, //OCTET STRING
		std::byte{3}, //octet string length
		std::byte{4}, std::byte{5}, std::byte{6},

		std::byte{}, std::byte{2} //some trailing garbage
	};
	obj.serialized_data = asn1_data;

	ASSERT_THROW((void)get_page_hashes<typename TestFixture::target_range_type>(this->authenticode),
		pe_bliss::pe_error);
}
