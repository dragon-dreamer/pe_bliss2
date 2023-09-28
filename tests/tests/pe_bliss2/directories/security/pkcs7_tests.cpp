#include "pe_bliss2/security/pkcs7/pkcs7.h"

#include <exception>

#include "gtest/gtest.h"

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/pkcs7/signer_info_ref.h"

#include "simple_asn1/crypto/algorithms.h"
#include "simple_asn1/crypto/pkcs7/authenticode/types.h"
#include "simple_asn1/crypto/pkcs7/cms/types.h"
#include "simple_asn1/crypto/pkcs7/types.h"
#include "simple_asn1/crypto/tst/types.h"

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
class Pkcs7Test : public testing::Test
{
public:
	using range_type = typename T::range_type;
	using pkcs7_type = typename T::type;

public:
	pkcs7_type message;
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

TYPED_TEST_SUITE(Pkcs7Test, tested_pkcs7_types);

TYPED_TEST(Pkcs7Test, GetSignerCount)
{
	ASSERT_EQ(this->message.get_signer_count(), 0u);

	this->message.get_content_info().data.signer_infos.emplace_back();

	ASSERT_EQ(this->message.get_signer_count(), 1u);
}

TYPED_TEST(Pkcs7Test, GetSigner)
{
	ASSERT_THROW((void)this->message.get_signer(0u), std::exception);

	auto& signer = this->message.get_content_info().data.signer_infos.emplace_back();

	auto ref = this->message.get_signer(0u);
	ASSERT_EQ(&ref.get_underlying(), &signer);
}

TYPED_TEST(Pkcs7Test, GeRawSignedContent)
{
	if constexpr (TestFixture::pkcs7_type::contains_pkcs7_signer_info)
	{
		const std::vector<std::byte> type_value{ std::byte{1} };
		const std::vector<std::byte> digest{ std::byte{2} };

		this->message.get_content_info().data.content_info.content.type_value.raw = type_value;
		this->message.get_content_info().data.content_info.content.digest.raw = digest;
		const auto content = this->message.get_raw_signed_content();

		ASSERT_EQ(content.size(), 2u);
		ASSERT_EQ(content[0].data(),
			this->message.get_content_info().data.content_info.content.type_value.raw.data());
		ASSERT_EQ(content[1].data(),
			this->message.get_content_info().data.content_info.content.digest.raw.data());
	}
	else
	{
		static_assert(TestFixture::pkcs7_type::contains_cms_signer_info);

		const std::vector<std::byte> info{
			std::byte{}, //TAGGED type (not checked)
			std::byte{20}, //TAGGED length
			std::byte{}, //OCTET_STRING type (not checked)
			std::byte{0x80 + 3}, //OCTET_STRING length
			std::byte{}, std::byte{}, std::byte{}, //extra length bytes

			std::byte{}, std::byte{}, std::byte{}, //value
		};

		this->message.get_content_info().data.content_info.info.raw = info;
		const auto content = this->message.get_raw_signed_content();
		ASSERT_EQ(content.size(), 1u);
		ASSERT_EQ(content[0].data(),
			this->message.get_content_info().data.content_info.info.raw.data() + 7u);


		const std::vector<std::byte> cut_info{
			std::byte{}, //TAGGED type (not checked)
			std::byte{20}, //TAGGED length
			std::byte{}, //OCTET_STRING type (not checked)
			std::byte{0x80 + 3}, //OCTET_STRING length
			std::byte{}
		};
		this->message.get_content_info().data.content_info.info.raw = cut_info;
		ASSERT_THROW((void)this->message.get_raw_signed_content(), pe_bliss::pe_error);
	}
}

TYPED_TEST(Pkcs7Test, CalculateMessageDigest)
{
	const std::vector<std::byte> raw_signed_content1{ std::byte{1}, std::byte{2}, std::byte{3} };
	const std::vector<std::byte> raw_signed_content2{ std::byte{4}, std::byte{5} };

	signer_info_ref_pkcs7<vector_range_type>::signer_info_type signer_info{};
	signer_info_ref_pkcs7<vector_range_type> signer_info_ref{ signer_info };

	//No hash algorithm specified - throws
	ASSERT_THROW((void)calculate_message_digest(this->message, signer_info_ref),
		pe_bliss::pe_error);

	const std::vector<std::uint32_t> md5_alg(asn1::crypto::hash::id_md5.cbegin(),
		asn1::crypto::hash::id_md5.cend());
	signer_info.digest_algorithm.algorithm.container = md5_alg;

	const std::vector<std::byte> md5_hash{ std::byte{0x7c}, std::byte{0xfd},
		std::byte{0xd0}, std::byte{0x78}, std::byte{0x89}, std::byte{0xb3},
		std::byte{0x29}, std::byte{0x5d}, std::byte{0x6a}, std::byte{0x55},
		std::byte{0x09}, std::byte{0x14}, std::byte{0xab}, std::byte{0x35},
		std::byte{0xe0}, std::byte{0x68} };

	if constexpr (TestFixture::pkcs7_type::contains_pkcs7_signer_info)
	{
		const std::vector<std::byte> type_value{ std::byte{1}, std::byte{2}, std::byte{3} };
		const std::vector<std::byte> digest{ std::byte{4}, std::byte{5} };
		this->message.get_content_info().data.content_info.content.type_value.raw = type_value;
		this->message.get_content_info().data.content_info.content.digest.raw = digest;

		ASSERT_EQ(calculate_message_digest(this->message, signer_info_ref), md5_hash);
	}
	else
	{
		const std::vector<std::byte> info{
			std::byte{}, //TAGGED type (not checked)
			std::byte{20}, //TAGGED length
			std::byte{}, //OCTET_STRING type (not checked)
			std::byte{1}, //OCTET_STRING length

			std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5} //value
		};
		this->message.get_content_info().data.content_info.info.raw = info;

		ASSERT_EQ(calculate_message_digest(this->message, signer_info_ref), md5_hash);
	}
}
