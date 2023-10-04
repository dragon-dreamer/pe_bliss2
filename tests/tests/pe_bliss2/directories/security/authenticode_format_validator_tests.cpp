#include "pe_bliss2/security/authenticode_format_validator.h"

#include <cstdint>
#include <optional>
#include <vector>

#include "gtest/gtest.h"

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/authenticode_format_validator_errc.h"
#include "pe_bliss2/security/authenticode_pkcs7.h"
#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/pkcs7/pkcs7_format_validator.h"

#include "simple_asn1/crypto/pkcs7/authenticode/oids.h"
#include "simple_asn1/types.h"

#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::security;
using namespace pe_bliss::security::pkcs7;

namespace
{
template<typename T>
class AuthenticodeFormatValidatorTest : public testing::Test
{
public:
	using range_type = T;
	using map_type = attribute_map<range_type>;
	using message_type = authenticode_pkcs7<range_type>;

	auto& add_signer_info()
	{
		return message.get_content_info().data.signer_infos.emplace_back();
	}

	auto& add_digest_algorithm()
	{
		return message.get_content_info().data.digest_algorithms.emplace_back();
	}

public:
	map_type map;
	message_type message{};
};

using tested_range_types = ::testing::Types<
	vector_range_type, span_range_type>;
} //namespace

TYPED_TEST_SUITE(AuthenticodeFormatValidatorTest, tested_range_types);

TYPED_TEST(AuthenticodeFormatValidatorTest, Empty)
{
	pe_bliss::error_list errors;
	validate_autenticode_format(this->message, errors);
	expect_contains_errors(errors,
		pkcs7_format_validator_errc::invalid_signed_data_oid,
		pkcs7_format_validator_errc::invalid_signed_data_version,
		pkcs7_format_validator_errc::invalid_signer_count);
}

TYPED_TEST(AuthenticodeFormatValidatorTest, EmptySigner)
{
	this->add_signer_info();
	this->add_digest_algorithm();
	pe_bliss::error_list errors;
	validate_autenticode_format(this->message, errors);
	expect_contains_errors(errors,
		pkcs7_format_validator_errc::invalid_signed_data_oid,
		pkcs7_format_validator_errc::invalid_signed_data_version,
		pkcs7_format_validator_errc::invalid_signer_info_version,
		authenticode_format_validator_errc::invalid_content_info_oid,
		authenticode_format_validator_errc::invalid_type_value_type);
}

TYPED_TEST(AuthenticodeFormatValidatorTest, EmptySignerValidContentTypeOid)
{
	this->add_signer_info();
	this->add_digest_algorithm();

	const std::vector<std::uint32_t> content_type_oid{
		asn1::crypto::pkcs7::authenticode::oid_spc_indirect_data_content.cbegin(),
		asn1::crypto::pkcs7::authenticode::oid_spc_indirect_data_content.cend()
	};
	this->message.get_content_info().data.content_info.content_type.container
		= content_type_oid;

	pe_bliss::error_list errors;
	validate_autenticode_format(this->message, errors);
	expect_contains_errors(errors,
		pkcs7_format_validator_errc::invalid_signed_data_oid,
		pkcs7_format_validator_errc::invalid_signed_data_version,
		pkcs7_format_validator_errc::invalid_signer_info_version,
		authenticode_format_validator_errc::invalid_type_value_type);
}

TYPED_TEST(AuthenticodeFormatValidatorTest, EmptySignerValidOids)
{
	this->add_signer_info();
	this->add_digest_algorithm();

	const std::vector<std::uint32_t> content_type_oid{
		asn1::crypto::pkcs7::authenticode::oid_spc_indirect_data_content.cbegin(),
		asn1::crypto::pkcs7::authenticode::oid_spc_indirect_data_content.cend()
	};
	this->message.get_content_info().data.content_info.content_type.container
		= content_type_oid;

	const std::vector<std::uint32_t> type_value_oid{
		asn1::crypto::pkcs7::authenticode::oid_spc_pe_image_data.cbegin(),
		asn1::crypto::pkcs7::authenticode::oid_spc_pe_image_data.cend()
	};
	this->message.get_content_info().data.content_info
		.content.type_value.value.type.container = type_value_oid;

	pe_bliss::error_list errors;
	validate_autenticode_format(this->message, errors);
	expect_contains_errors(errors,
		pkcs7_format_validator_errc::invalid_signed_data_oid,
		pkcs7_format_validator_errc::invalid_signed_data_version,
		pkcs7_format_validator_errc::invalid_signer_info_version);
}

TYPED_TEST(AuthenticodeFormatValidatorTest, DigestAlgorithmMismatch)
{
	const std::vector<std::uint32_t> digest_algorithm{ 1, 2, 3 };
	auto& signer_info = this->add_signer_info();
	signer_info.digest_algorithm.algorithm.container = digest_algorithm;

	this->add_digest_algorithm();

	pe_bliss::error_list errors;
	validate_autenticode_format(this->message, errors);
	expect_contains_errors(errors,
		pkcs7_format_validator_errc::invalid_signed_data_oid,
		pkcs7_format_validator_errc::invalid_signed_data_version,
		pkcs7_format_validator_errc::invalid_signer_info_version,
		pkcs7_format_validator_errc::non_matching_digest_algorithm,
		authenticode_format_validator_errc::invalid_content_info_oid,
		authenticode_format_validator_errc::invalid_type_value_type,
		authenticode_format_validator_errc::non_matching_type_value_digest_algorithm);
}

TYPED_TEST(AuthenticodeFormatValidatorTest, EmptyAttributes)
{
	pe_bliss::error_list errors;
	std::optional<asn1::utc_time> signing_time;
	validate_autenticode_format(this->map, signing_time, errors);
	expect_contains_errors(errors,
		pkcs7_format_validator_errc::absent_message_digest,
		pkcs7_format_validator_errc::absent_content_type);
}

TYPED_TEST(AuthenticodeFormatValidatorTest, AttributesInvalidContentType)
{
	const std::vector<std::byte> valid_oid{ std::byte{0x06u}, std::byte{0x08u},
	std::byte{0x2bu}, std::byte{0x06u}, std::byte{0x01u}, std::byte{0x05u},
	std::byte{0x05u}, std::byte{0x07u}, std::byte{0x30u}, std::byte{0x01u} };
	const std::vector<typename TestFixture::range_type> test_single_data{ valid_oid };
	const std::vector<std::uint32_t> oid_content_type(
		asn1::crypto::pkcs7::oid_content_type.cbegin(),
		asn1::crypto::pkcs7::oid_content_type.cend());
	this->map.get_map().try_emplace(oid_content_type, test_single_data);

	pe_bliss::error_list errors;
	std::optional<asn1::utc_time> signing_time;
	validate_autenticode_format(this->map, signing_time, errors);
	expect_contains_errors(errors,
		pkcs7_format_validator_errc::absent_message_digest,
		pkcs7_format_validator_errc::invalid_content_type);
}

TYPED_TEST(AuthenticodeFormatValidatorTest, AttributesValidContentType)
{
	constexpr auto oid_array = asn1::encode_oid<1u, 3u, 6u, 1u, 4u, 1u, 311u, 2u, 1u, 4u>();

	std::vector<std::byte> valid_oid{ std::byte{0x06u}, static_cast<std::byte>(oid_array.size()) };
	valid_oid.insert(valid_oid.end(),
		reinterpret_cast<const std::byte*>(oid_array.data()),
		reinterpret_cast<const std::byte*>(oid_array.data()) + oid_array.size());

	const std::vector<typename TestFixture::range_type> test_single_data{ valid_oid };
	const std::vector<std::uint32_t> oid_content_type(
		asn1::crypto::pkcs7::oid_content_type.cbegin(),
		asn1::crypto::pkcs7::oid_content_type.cend());
	this->map.get_map().try_emplace(oid_content_type, test_single_data);

	pe_bliss::error_list errors;
	std::optional<asn1::utc_time> signing_time;
	validate_autenticode_format(this->map, signing_time, errors);
	expect_contains_errors(errors,
		pkcs7_format_validator_errc::absent_message_digest);
}
