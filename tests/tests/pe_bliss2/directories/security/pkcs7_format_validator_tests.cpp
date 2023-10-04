#include "pe_bliss2/security/pkcs7/pkcs7_format_validator.h"

#include "gtest/gtest.h"

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/pkcs7/attribute_map.h"

#include "simple_asn1/crypto/pkcs7/authenticode/types.h"
#include "simple_asn1/crypto/pkcs7/cms/types.h"
#include "simple_asn1/crypto/pkcs7/oids.h"
#include "simple_asn1/crypto/pkcs7/types.h"
#include "simple_asn1/crypto/crypto_common_types.h"
#include "simple_asn1/crypto/tst/types.h"
#include "simple_asn1/types.h"

#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::security;
using namespace pe_bliss::security::pkcs7;

namespace
{
template<template<typename> typename Pkcs7Type, typename RangeType>
struct pkcs7_types
{
	using range_type = RangeType;
	using type = Pkcs7Type<range_type>;
};

template<typename T>
class Pkcs7FormatValidatorTest : public testing::Test
{
public:
	using range_type = typename T::range_type;
	using pkcs7_type = typename T::type;

public:
	pe_bliss::error_list validate() const
	{
		pe_bliss::error_list errors;
		pe_bliss::security::pkcs7::validate(message, errors);
		return errors;
	}

	void fill_oid()
	{
		static const std::vector<std::uint32_t> oid_signed_data{
			asn1::crypto::pkcs7::oid_signed_data.cbegin(),
			asn1::crypto::pkcs7::oid_signed_data.cend() };
		message.get_content_info().content_type.container = oid_signed_data;
	}

	void fill_signed_data_version()
	{
		message.get_content_info().data.version = signed_data_version;
	}

	auto& add_signer_info()
	{
		return message.get_content_info().data.signer_infos.emplace_back();
	}

	auto& add_digest_algorithm()
	{
		return message.get_content_info().data.digest_algorithms.emplace_back();
	}

public:
	pkcs7_type message{};
};

template<typename RangeType>
using authenticode_signature_cms_info_ms_bug_workaround_type
	= pe_bliss::security::pkcs7::pkcs7<RangeType,
		asn1::crypto::pkcs7::cms::ms_bug_workaround::content_info_base<
		asn1::crypto::tst::encap_tst_info<RangeType>, RangeType>>;
template<typename RangeType>
using authenticode_signature_cms_info_type
	= pe_bliss::security::pkcs7::pkcs7<RangeType,
		asn1::crypto::pkcs7::cms::content_info_base<
		asn1::crypto::tst::encap_tst_info<RangeType>, RangeType>>;

template<typename RangeType>
using authenticode_pkcs7 = pe_bliss::security::pkcs7::pkcs7<RangeType,
	asn1::crypto::pkcs7::authenticode::content_info<RangeType>>;

using tested_pkcs7_types = ::testing::Types<
	pkcs7_types<authenticode_signature_cms_info_ms_bug_workaround_type, vector_range_type>,
	pkcs7_types<authenticode_signature_cms_info_ms_bug_workaround_type, span_range_type>,
	pkcs7_types<authenticode_signature_cms_info_type, vector_range_type>,
	pkcs7_types<authenticode_signature_cms_info_type, span_range_type>,
	pkcs7_types<authenticode_pkcs7, vector_range_type>,
	pkcs7_types<authenticode_pkcs7, span_range_type>>;
} //namespace

TYPED_TEST_SUITE(Pkcs7FormatValidatorTest, tested_pkcs7_types);

TYPED_TEST(Pkcs7FormatValidatorTest, Empty)
{
	expect_contains_errors(this->validate(),
		pkcs7_format_validator_errc::invalid_signed_data_oid,
		pkcs7_format_validator_errc::invalid_signed_data_version);
}

TYPED_TEST(Pkcs7FormatValidatorTest, ValidOidNoSigners)
{
	this->fill_oid();
	expect_contains_errors(this->validate(),
		pkcs7_format_validator_errc::invalid_signed_data_version);
}

TYPED_TEST(Pkcs7FormatValidatorTest, ValidOidAndSignedDataVerNoSigners)
{
	this->fill_oid();
	this->fill_signed_data_version();
	expect_contains_errors(this->validate());

	pe_bliss::error_list errors;
	pe_bliss::security::pkcs7::validate(this->message, errors, signed_data_version + 1u);
	expect_contains_errors(errors,
		pkcs7_format_validator_errc::invalid_signed_data_version);
}

TYPED_TEST(Pkcs7FormatValidatorTest, MismatchingSignerCount)
{
	this->fill_oid();
	this->fill_signed_data_version();
	this->add_signer_info();
	expect_contains_errors(this->validate(),
		pkcs7_format_validator_errc::invalid_signer_count);
}

TYPED_TEST(Pkcs7FormatValidatorTest, InvalidSignerVersion)
{
	this->fill_oid();
	this->fill_signed_data_version();
	this->add_signer_info();
	this->add_digest_algorithm();
	expect_contains_errors(this->validate(),
		pkcs7_format_validator_errc::invalid_signer_info_version);
}

TYPED_TEST(Pkcs7FormatValidatorTest, MismatchingDigestAlgorithm)
{
	this->fill_oid();
	this->fill_signed_data_version();

	const std::vector<std::uint32_t> digest_algorithm{ 1, 2, 3 };
	auto& signer_info = this->add_signer_info();
	signer_info.digest_algorithm.algorithm.container = digest_algorithm;
	signer_info.version = pe_bliss::security::pkcs7::impl::signer_info_version;

	this->add_digest_algorithm();
	expect_contains_errors(this->validate(),
		pkcs7_format_validator_errc::non_matching_digest_algorithm);
}

TYPED_TEST(Pkcs7FormatValidatorTest, Valid)
{
	this->fill_oid();
	this->fill_signed_data_version();

	const std::vector<std::uint32_t> digest_algorithm{ 1, 2, 3 };
	auto& signer_info = this->add_signer_info();
	signer_info.digest_algorithm.algorithm.container = digest_algorithm;
	signer_info.version = pe_bliss::security::pkcs7::impl::signer_info_version;

	this->add_digest_algorithm().algorithm.container = digest_algorithm;
	expect_contains_errors(this->validate());
}

namespace
{
template<typename T>
class AuthenticatedAttributesValidatorTest : public testing::Test
{
public:
	using range_type = T;
	using map_type = attribute_map<range_type>;

public:
	map_type map;
};

using tested_range_types = ::testing::Types<
	vector_range_type, span_range_type>;
} //namespace

TYPED_TEST_SUITE(AuthenticatedAttributesValidatorTest, tested_range_types);

TYPED_TEST(AuthenticatedAttributesValidatorTest, Empty)
{
	std::optional<asn1::utc_time> signing_time;
	std::optional<asn1::crypto::object_identifier_type> content_type;
	pe_bliss::error_list errors;

	validate_authenticated_attributes(
		this->map, signing_time, content_type,errors);
	expect_contains_errors(errors,
		pkcs7_format_validator_errc::absent_message_digest,
		pkcs7_format_validator_errc::absent_content_type);
	ASSERT_FALSE(signing_time);
	ASSERT_FALSE(content_type);
};

TYPED_TEST(AuthenticatedAttributesValidatorTest, ValidMessageDigest)
{
	const std::vector<std::byte> test_data1{ std::byte{1}, std::byte{2} };
	const std::vector<typename TestFixture::range_type> test_single_data{ test_data1 };
	const std::vector<std::uint32_t> oid_message_digest(
		asn1::crypto::pkcs7::oid_message_digest.cbegin(),
		asn1::crypto::pkcs7::oid_message_digest.cend());
	this->map.get_map().try_emplace(oid_message_digest, test_single_data);

	std::optional<asn1::utc_time> signing_time;
	std::optional<asn1::crypto::object_identifier_type> content_type;
	pe_bliss::error_list errors;

	validate_authenticated_attributes(
		this->map, signing_time, content_type, errors);
	expect_contains_errors(errors,
		pkcs7_format_validator_errc::absent_content_type);
	ASSERT_FALSE(signing_time);
	ASSERT_FALSE(content_type);
}

TYPED_TEST(AuthenticatedAttributesValidatorTest, InvalidContentType)
{
	const std::vector<std::byte> test_data1{ std::byte{1}, std::byte{2} };
	const std::vector<typename TestFixture::range_type> test_single_data{ test_data1 };
	const std::vector<std::uint32_t> oid_content_type(
		asn1::crypto::pkcs7::oid_content_type.cbegin(),
		asn1::crypto::pkcs7::oid_content_type.cend());
	this->map.get_map().try_emplace(oid_content_type, test_single_data);

	std::optional<asn1::utc_time> signing_time;
	std::optional<asn1::crypto::object_identifier_type> content_type;
	pe_bliss::error_list errors;

	validate_authenticated_attributes(
		this->map, signing_time, content_type, errors);
	expect_contains_errors(errors,
		pkcs7_format_validator_errc::absent_message_digest,
		pkcs7_format_validator_errc::invalid_content_type);
	ASSERT_FALSE(signing_time);
	ASSERT_FALSE(content_type);
}

TYPED_TEST(AuthenticatedAttributesValidatorTest, ValidContentType)
{
	const std::vector<std::byte> valid_oid{ std::byte{0x06u}, std::byte{0x08u},
		std::byte{0x2bu}, std::byte{0x06u}, std::byte{0x01u}, std::byte{0x05u},
		std::byte{0x05u}, std::byte{0x07u}, std::byte{0x30u}, std::byte{0x01u} };
	const std::vector<typename TestFixture::range_type> test_single_data{ valid_oid };
	const std::vector<std::uint32_t> oid_content_type(
		asn1::crypto::pkcs7::oid_content_type.cbegin(),
		asn1::crypto::pkcs7::oid_content_type.cend());
	this->map.get_map().try_emplace(oid_content_type, test_single_data);

	std::optional<asn1::utc_time> signing_time;
	std::optional<asn1::crypto::object_identifier_type> content_type;
	pe_bliss::error_list errors;

	validate_authenticated_attributes(
		this->map, signing_time, content_type, errors);
	expect_contains_errors(errors,
		pkcs7_format_validator_errc::absent_message_digest);
	ASSERT_FALSE(signing_time);
	ASSERT_TRUE(content_type);
	ASSERT_EQ(content_type->container, (std::vector<std::uint32_t>{1, 3, 6, 1, 5, 5, 7, 48, 1}));
}

TYPED_TEST(AuthenticatedAttributesValidatorTest, InvalidSigningTime)
{
	const std::vector<std::byte> test_data1{ std::byte{1}, std::byte{2} };
	const std::vector<typename TestFixture::range_type> test_single_data{ test_data1 };
	const std::vector<std::uint32_t> oid_signing_time(
		asn1::crypto::pkcs7::oid_signing_time.cbegin(),
		asn1::crypto::pkcs7::oid_signing_time.cend());
	this->map.get_map().try_emplace(oid_signing_time, test_single_data);

	std::optional<asn1::utc_time> signing_time;
	std::optional<asn1::crypto::object_identifier_type> content_type;
	pe_bliss::error_list errors;

	validate_authenticated_attributes(
		this->map, signing_time, content_type, errors);
	expect_contains_errors(errors,
		pkcs7_format_validator_errc::absent_message_digest,
		pkcs7_format_validator_errc::absent_content_type,
		pkcs7_format_validator_errc::invalid_signing_time);
	ASSERT_FALSE(signing_time);
	ASSERT_FALSE(content_type);
}

TYPED_TEST(AuthenticatedAttributesValidatorTest, ValidSigningTime)
{
	const std::vector<std::byte> valid_utc_time{ std::byte{0x17u}, std::byte{0x0du},
		std::byte{0x39u}, std::byte{0x36u}, std::byte{0x30u}, std::byte{0x34u},
		std::byte{0x31u}, std::byte{0x35u}, std::byte{0x32u}, std::byte{0x30u},
		std::byte{0x33u}, std::byte{0x30u}, std::byte{0x30u}, std::byte{0x31u},
		std::byte{0x5au} };
	const std::vector<typename TestFixture::range_type> test_single_data{ valid_utc_time };
	const std::vector<std::uint32_t> oid_signing_time(
		asn1::crypto::pkcs7::oid_signing_time.cbegin(),
		asn1::crypto::pkcs7::oid_signing_time.cend());
	this->map.get_map().try_emplace(oid_signing_time, test_single_data);

	std::optional<asn1::utc_time> signing_time;
	std::optional<asn1::crypto::object_identifier_type> content_type;
	pe_bliss::error_list errors;

	validate_authenticated_attributes(
		this->map, signing_time, content_type, errors);
	expect_contains_errors(errors,
		pkcs7_format_validator_errc::absent_message_digest,
		pkcs7_format_validator_errc::absent_content_type);
	ASSERT_TRUE(signing_time);
	ASSERT_FALSE(content_type);
	ASSERT_EQ(signing_time->year, 96u);
	ASSERT_EQ(signing_time->month, 4u);
	ASSERT_EQ(signing_time->day, 15u);
	ASSERT_EQ(signing_time->hour, 20u);
	ASSERT_EQ(signing_time->minute, 30u);
	ASSERT_EQ(signing_time->second, 1u);
}
