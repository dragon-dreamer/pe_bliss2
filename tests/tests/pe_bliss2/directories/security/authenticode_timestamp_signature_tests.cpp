#include "pe_bliss2/security/authenticode_timestamp_signature.h"

#include <cstddef>
#include <span>
#include <vector>

#include "gtest/gtest.h"

#include "simple_asn1/crypto/pkcs7/authenticode/oids.h"
#include "simple_asn1/crypto/pkcs9/oids.h"

#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::security;

namespace
{
template<typename TargetRangeType, typename SourceRangeType>
struct RangeTypes
{
	using target_range_type = TargetRangeType;
	using source_range_type = SourceRangeType;
};

template<typename T>
class TimestampSignatureLoaderTest : public testing::Test
{
public:
	using target_range_type = typename T::target_range_type;
	using source_range_type = typename T::source_range_type;

public:
	pkcs7::attribute_map<source_range_type> map;
};

using tested_range_types = ::testing::Types<
	RangeTypes<vector_range_type, span_range_type>,
	RangeTypes<vector_range_type, vector_range_type>,
	RangeTypes<span_range_type, span_range_type>,
	RangeTypes<span_range_type, vector_range_type>>;
} // namespace

TYPED_TEST_SUITE(TimestampSignatureLoaderTest, tested_range_types);

TYPED_TEST(TimestampSignatureLoaderTest, LoadEmpty)
{
	using target_range_type = typename TestFixture::target_range_type;
	auto result = load_timestamp_signature<target_range_type>(this->map);
	ASSERT_FALSE(result);
}

TYPED_TEST(TimestampSignatureLoaderTest, LoadInvalidAsn1)
{
	const std::vector<std::uint32_t> oid_nested_signature_attribute(
		asn1::crypto::pkcs7::authenticode::oid_nested_signature_attribute.cbegin(),
		asn1::crypto::pkcs7::authenticode::oid_nested_signature_attribute.cend());

	const std::vector<std::byte> invalid_asn1_der{ std::byte{1} };
	const std::vector<typename TestFixture::source_range_type> attr_data{ invalid_asn1_der };

	auto tester = [&](const auto& attr) {
		const std::vector<std::uint32_t> attr_copy(attr.cbegin(), attr.cend());
		this->map.get_map().try_emplace(attr_copy, attr_data);
		expect_throw_pe_error([this] {
			(void)load_timestamp_signature<typename TestFixture::target_range_type>(this->map);
		}, timestamp_signature_loader_errc::invalid_timestamp_signature_asn1_der);
		this->map.get_map().clear();
	};

	tester(asn1::crypto::pkcs9::oid_timestamp_token);
	tester(asn1::crypto::pkcs7::authenticode::oid_spc_time_stamp_token);
	tester(asn1::crypto::pkcs9::oid_counter_signature);
}

TYPED_TEST(TimestampSignatureLoaderTest, LoadTimestampCounterSignature)
{
	const std::vector<std::uint32_t> attr(
		asn1::crypto::pkcs9::oid_counter_signature.cbegin(),
		asn1::crypto::pkcs9::oid_counter_signature.cend());
	std::vector<std::byte> signer_info{
		std::byte{ 0x30u }, //SEQUENCE
		std::byte{36}, //SEQUENCE length

		std::byte{ 0x02u }, //INTEGER
		std::byte{1}, //INTEGER length
		std::byte{15},

		//Issuer and serial number
		std::byte{ 0x30u }, //SEQUENCE
		std::byte{16}, //SEQUENCE length

		  std::byte{ 0x30u }, //SEQUENCE_OF
		  std::byte{11}, //SEQUENCE_OF length

		    std::byte{ 0x31u }, //SET_OF
		    std::byte{9}, //SET_OF length

		      std::byte{ 0x30u }, //SEQUENCE
		      std::byte{7}, //SEQUENCE length

			    std::byte{ 0x06u }, //OBJECT IDENTIFIER
		        std::byte{2}, //OBJECT IDENTIFIER length
		        std::byte{ 0x2au }, std::byte{ 0x03u },

				std::byte{ 0x02u }, //ANY
				std::byte{1}, //ANY length
				std::byte{10},

		  std::byte{ 0x02u }, //INTEGER
		  std::byte{1}, //INTEGER length
		  std::byte{5},

		//digest algorithm identifier
		std::byte{ 0x30u }, //SEQUENCE
		std::byte{4}, //SEQUENCE length

		  std::byte{ 0x06u }, //OBJECT IDENTIFIER
		  std::byte{2}, //OBJECT IDENTIFIER length
		  std::byte{ 0x2au }, std::byte{ 0x03u },

		//digestEncryptionAlgorithm
		std::byte{ 0x30u }, //SEQUENCE
		std::byte{4}, //SEQUENCE length

		  std::byte{ 0x06u }, //OBJECT IDENTIFIER
		  std::byte{2}, //OBJECT IDENTIFIER length
		  std::byte{ 0x2au }, std::byte{ 0x03u },

		std::byte{ 0x04u }, //OCTET STRING
		std::byte{1}, //OCTET STRING length
		std::byte{2},
	};

	for (int i = 0; i < 2; ++i)
	{
		const std::vector<typename TestFixture::source_range_type> attr_data{ signer_info };
		this->map.get_map().try_emplace(attr, attr_data);
		const auto result = load_timestamp_signature<
			typename TestFixture::target_range_type>(this->map);
		ASSERT_TRUE(result);

		const auto* underlying = std::get_if<typename authenticode_timestamp_signature<
			typename TestFixture::target_range_type>::signer_info_type>(&result->get_underlying_type());
		ASSERT_NE(underlying, nullptr);
		ASSERT_EQ(underlying->version, 15);

		this->map.get_map().clear();
		signer_info.emplace_back(std::byte{}); //Add trailing nullbyte
	}

	signer_info.emplace_back(std::byte{ 1 }); //Add trailing non-zero byte
	const std::vector<typename TestFixture::source_range_type> attr_data{ signer_info };
	this->map.get_map().try_emplace(attr, attr_data);
	expect_throw_pe_error([this] {
		(void)load_timestamp_signature<typename TestFixture::target_range_type>(this->map);
	}, timestamp_signature_loader_errc::invalid_timestamp_signature_asn1_der);
}

TYPED_TEST(TimestampSignatureLoaderTest, LoadMicrosoftCmsSignature)
{
	const std::vector<std::uint32_t> attr(
		asn1::crypto::pkcs7::authenticode::oid_spc_time_stamp_token.cbegin(),
		asn1::crypto::pkcs7::authenticode::oid_spc_time_stamp_token.cend());
	std::vector<std::byte> signer_info{
		std::byte{ 0x30u }, //SEQUENCE
		std::byte{71}, //SEQUENCE length

		std::byte{ 0x06u }, //OBJECT IDENTIFIER
		std::byte{2}, //OBJECT IDENTIFIER length
		std::byte{ 0x2au }, std::byte{ 0x03u },

		std::byte{ 0xa0u }, //TAGGED
		std::byte{65}, //TAGGED length

		  std::byte{ 0x30u }, //SEQUENCE
		  std::byte{63}, //SEQUENCE length

		  std::byte{ 0x02u }, //INTEGER
		  std::byte{1}, //INTEGER length
		  std::byte{15},

		  //digest algorithm identifiers
		  std::byte{ 0x31u }, //SET_OF
		  std::byte{6}, //SET_OF length

		    //digest algorithm identifier
		    std::byte{ 0x30u }, //SEQUENCE
		    std::byte{4}, //SEQUENCE length

			  std::byte{ 0x06u }, //OBJECT IDENTIFIER
			  std::byte{2}, //OBJECT IDENTIFIER length
			  std::byte{ 0x2au }, std::byte{ 0x03u },

		  //encap_tst_info
		  std::byte{ 0x30u }, //SEQUENCE
		  std::byte{48}, //SEQUENCE length

			std::byte{ 0x06u }, //OBJECT IDENTIFIER
			std::byte{2}, //OBJECT IDENTIFIER length
			std::byte{ 0x2au }, std::byte{ 0x03u },

		    std::byte{ 0xa0u }, //TAGGED
		    std::byte{42}, //TAGGED length

			std::byte{ 0x04u }, //OCTET STRING
			std::byte{40}, //OCTET STRING length
			
			  //tst_info
		      std::byte{ 0x30u }, //SEQUENCE
		      std::byte{38}, //SEQUENCE length

		        std::byte{ 0x02u }, //INTEGER
		        std::byte{1}, //INTEGER length
		        std::byte{123},

			    std::byte{ 0x06u }, //OBJECT IDENTIFIER
			    std::byte{2}, //OBJECT IDENTIFIER length
			    std::byte{ 0x2au }, std::byte{ 0x03u },

				//message_imprint
			    std::byte{ 0x30u }, //SEQUENCE
			    std::byte{9}, //SEQUENCE length

		          //algorithm identifier
		          std::byte{ 0x30u }, //SEQUENCE
		          std::byte{4}, //SEQUENCE length

				    std::byte{ 0x06u }, //OBJECT IDENTIFIER
				    std::byte{2}, //OBJECT IDENTIFIER length
				    std::byte{ 0x2au }, std::byte{ 0x03u },

			      std::byte{ 0x04u }, //OCTET STRING
			      std::byte{1}, //OCTET STRING length
				  std::byte{2},

				std::byte{ 0x02u }, //INTEGER
				std::byte{1}, //INTEGER length
				std::byte{5},

				std::byte{ 0x18u }, //GENERALIZED TIME
				std::byte{15}, //GENERALIZED TIME length
				std::byte{'2'}, std::byte{'0'}, std::byte{'0'}, std::byte{'0'},
				std::byte{'1'}, std::byte{'2'}, std::byte{'3'}, std::byte{'1'},
				std::byte{'2'}, std::byte{'3'}, std::byte{'5'}, std::byte{'9'},
				std::byte{'5'}, std::byte{'9'}, std::byte{'Z'},

		  //Signer infos
		  std::byte{ 0x31u }, //SET_OF
		  std::byte{0}, //SET_OF length
	};

	for (int i = 0; i < 2; ++i)
	{
		const std::vector<typename TestFixture::source_range_type> attr_data{ signer_info };
		this->map.get_map().try_emplace(attr, attr_data);
		const auto result = load_timestamp_signature<
			typename TestFixture::target_range_type>(this->map);
		ASSERT_TRUE(result);

		const auto* underlying = std::get_if<typename authenticode_timestamp_signature<
			typename TestFixture::target_range_type>::cms_info_ms_bug_workaround_type>(
				&result->get_underlying_type());
		ASSERT_NE(underlying, nullptr);
		ASSERT_EQ(underlying->get_content_info().data.content_info.info.value.version, 123);

		this->map.get_map().clear();
		signer_info.emplace_back(std::byte{}); //Add trailing nullbyte
	}

	signer_info.emplace_back(std::byte{ 1 }); //Add trailing non-zero byte
	const std::vector<typename TestFixture::source_range_type> attr_data{ signer_info };
	this->map.get_map().try_emplace(attr, attr_data);
	expect_throw_pe_error([this] {
		(void)load_timestamp_signature<typename TestFixture::target_range_type>(this->map);
	}, timestamp_signature_loader_errc::invalid_timestamp_signature_asn1_der);
}
