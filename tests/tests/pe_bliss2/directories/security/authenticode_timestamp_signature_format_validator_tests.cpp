#include "pe_bliss2/security/authenticode_timestamp_signature_format_validator.h"

#include <cstddef>
#include <variant>
#include <vector>

#include "gtest/gtest.h"

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/authenticode_format_validator_errc.h"
#include "pe_bliss2/security/pkcs7/pkcs7_format_validator.h"
#include "pe_bliss2/security/pkcs7/signer_info.h"

#include "simple_asn1/crypto/algorithms.h"
#include "simple_asn1/crypto/pkcs7/oids.h"
#include "simple_asn1/crypto/pkcs9/oids.h"

#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::security;
using namespace pe_bliss::security::pkcs7;

namespace
{
template<typename T>
class AuthenticodeTimestampSignatureFormatValidatorTest : public testing::Test
{
public:
	using range_type = T;

	const std::vector<std::byte> test_vector{ std::byte{1}, std::byte{2} };
	const std::vector<range_type> test_data{ test_vector };
	const std::vector<std::uint32_t> oid_message_digest{
		asn1::crypto::pkcs7::oid_message_digest.cbegin(),
		asn1::crypto::pkcs7::oid_message_digest.cend() };

	const std::vector<std::byte> valid_oid{ std::byte{0x06u}, std::byte{0x0bu},
		std::byte{0x2au}, std::byte{0x86u}, std::byte{0x48u}, std::byte{0x86u},
		std::byte{0xf7u}, std::byte{0x0du}, std::byte{0x01u}, std::byte{0x09u},
		std::byte{0x10u}, std::byte{0x01u}, std::byte{0x04u} };
	const std::vector<range_type> valid_oid_data{ valid_oid };
	const std::vector<std::byte> invalid_oid{
		std::byte{0x06u}, std::byte{0x01u}, std::byte{0x2au} };
	const std::vector<range_type> invalid_oid_data{ invalid_oid };
	const std::vector<std::uint32_t> oid_content_type{
		asn1::crypto::pkcs7::oid_content_type.cbegin(),
		asn1::crypto::pkcs7::oid_content_type.cend() };
};

using tested_range_types = ::testing::Types<
	vector_range_type, span_range_type>;
} // namespace

TYPED_TEST_SUITE(AuthenticodeTimestampSignatureFormatValidatorTest, tested_range_types);

TYPED_TEST(AuthenticodeTimestampSignatureFormatValidatorTest, EmptyAttributeMap)
{
	using range_type = typename TestFixture::range_type;
	attribute_map<range_type> authenticated_attributes;
	error_list errors;
	EXPECT_NO_THROW(validate_autenticode_timestamp_format(
		authenticated_attributes, errors));

	expect_contains_errors(errors,
		pkcs7_format_validator_errc::absent_message_digest,
		pkcs7_format_validator_errc::absent_content_type);
}

TYPED_TEST(AuthenticodeTimestampSignatureFormatValidatorTest, ValidAttributeMap)
{
	using range_type = typename TestFixture::range_type;
	attribute_map<range_type> authenticated_attributes;

	authenticated_attributes.get_map().try_emplace(
		this->oid_message_digest, this->test_data);
	          
	authenticated_attributes.get_map().try_emplace(
		this->oid_content_type, this->valid_oid_data);

	error_list errors;
	EXPECT_NO_THROW(validate_autenticode_timestamp_format(
		authenticated_attributes, errors));

	expect_contains_errors(errors);
}

TYPED_TEST(AuthenticodeTimestampSignatureFormatValidatorTest, ValidAttributeMapInvalidOid)
{
	using range_type = typename TestFixture::range_type;
	attribute_map<range_type> authenticated_attributes;

	authenticated_attributes.get_map().try_emplace(
		this->oid_message_digest, this->test_data);

	authenticated_attributes.get_map().try_emplace(
		this->oid_content_type, this->invalid_oid_data);

	error_list errors;
	EXPECT_NO_THROW(validate_autenticode_timestamp_format(
		authenticated_attributes, errors));

	expect_contains_errors(errors,
		pkcs7_format_validator_errc::invalid_content_type);
}

TYPED_TEST(AuthenticodeTimestampSignatureFormatValidatorTest, EmptyCms)
{
	using range_type = typename TestFixture::range_type;
	authenticode_signature_cms_info_ms_bug_workaround_type<range_type> signature{};

	error_list errors;
	EXPECT_NO_THROW(validate_autenticode_timestamp_format(signature, errors));

	expect_contains_errors(errors,
		pkcs7_format_validator_errc::invalid_signed_data_oid,
		pkcs7_format_validator_errc::invalid_signed_data_version,
		pkcs7_format_validator_errc::invalid_signer_count);
}

namespace
{
template<typename Signature>
void add_signer_info_and_algorithm(Signature& signature)
{
	auto& signer_info = signature.get_content_info().data.signer_infos.emplace_back();
	signer_info.digest_algorithm.algorithm.container.assign(
		asn1::crypto::signature::id_sha1_with_rsa_encryption.begin(),
		asn1::crypto::signature::id_sha1_with_rsa_encryption.end());
	signer_info.version = 1u;

	auto& algorithm = signature.get_content_info().data.digest_algorithms.emplace_back();
	algorithm.algorithm.container.assign(
		asn1::crypto::signature::id_sha1_with_rsa_encryption.begin(),
		asn1::crypto::signature::id_sha1_with_rsa_encryption.end());
}

template<typename Signature>
void add_tst_oid(Signature& signature)
{
	auto& signed_data_content_info = signature.get_content_info().data.content_info;
	signed_data_content_info.content_type.container.assign(
		asn1::crypto::pkcs9::oid_tst_info.begin(),
		asn1::crypto::pkcs9::oid_tst_info.end());
}

template<typename Signature>
void add_tst_version(Signature& signature)
{
	auto& signed_data_content_info = signature.get_content_info().data.content_info;
	auto& tst_info = signed_data_content_info.info.value;
	tst_info.version = 1u;
}

template<typename Signature>
void add_tst_accuracy(Signature& signature, int millis = 123, int micros = 456)
{
	auto& signed_data_content_info = signature.get_content_info().data.content_info;
	auto& tst_info = signed_data_content_info.info.value;
	auto& accuracy = tst_info.accuracy_val.emplace();
	accuracy.millis = millis;
	accuracy.micros = micros;
}

template<typename Signature>
Signature create_signature()
{
	Signature signature{};
	signature.get_content_info().data.version = 3u;
	signature.get_content_info().content_type.container.assign(
		asn1::crypto::pkcs7::oid_signed_data.begin(), asn1::crypto::pkcs7::oid_signed_data.end());
	return signature;
}
} // namespace

TYPED_TEST(AuthenticodeTimestampSignatureFormatValidatorTest, ValidCmsAndTst)
{
	using range_type = typename TestFixture::range_type;
	auto signature = create_signature<
		authenticode_signature_cms_info_ms_bug_workaround_type<range_type>>();

	add_signer_info_and_algorithm(signature);
	add_tst_oid(signature);
	add_tst_version(signature);

	error_list errors;
	EXPECT_NO_THROW(validate_autenticode_timestamp_format(signature, errors));

	expect_contains_errors(errors);
}

TYPED_TEST(AuthenticodeTimestampSignatureFormatValidatorTest, ValidCmsAndTstWithAccuracy)
{
	using range_type = typename TestFixture::range_type;
	auto signature = create_signature<
		authenticode_signature_cms_info_ms_bug_workaround_type<range_type>>();

	add_signer_info_and_algorithm(signature);
	add_tst_oid(signature);
	add_tst_version(signature);
	add_tst_accuracy(signature);

	error_list errors;
	EXPECT_NO_THROW(validate_autenticode_timestamp_format(signature, errors));

	expect_contains_errors(errors);
}

TYPED_TEST(AuthenticodeTimestampSignatureFormatValidatorTest, AbsentTstOid)
{
	using range_type = typename TestFixture::range_type;
	auto signature = create_signature<
		authenticode_signature_cms_info_ms_bug_workaround_type<range_type>>();

	add_signer_info_and_algorithm(signature);
	add_tst_version(signature);
	add_tst_accuracy(signature);

	error_list errors;
	EXPECT_NO_THROW(validate_autenticode_timestamp_format(signature, errors));

	expect_contains_errors(errors,
		authenticode_format_validator_errc::invalid_content_info_oid);
}

TYPED_TEST(AuthenticodeTimestampSignatureFormatValidatorTest, AbsentTstVersion)
{
	using range_type = typename TestFixture::range_type;
	auto signature = create_signature<
		authenticode_signature_cms_info_ms_bug_workaround_type<range_type>>();

	add_signer_info_and_algorithm(signature);
	add_tst_oid(signature);
	add_tst_accuracy(signature);

	error_list errors;
	EXPECT_NO_THROW(validate_autenticode_timestamp_format(signature, errors));

	expect_contains_errors(errors,
		authenticode_timestamp_signature_format_validator_errc::invalid_tst_info_version);
}

TYPED_TEST(AuthenticodeTimestampSignatureFormatValidatorTest, InvalidTstAccuracyMillis)
{
	using range_type = typename TestFixture::range_type;
	auto signature = create_signature<
		authenticode_signature_cms_info_ms_bug_workaround_type<range_type>>();

	add_signer_info_and_algorithm(signature);
	add_tst_oid(signature);
	add_tst_version(signature);
	add_tst_accuracy(signature, 1000);

	error_list errors;
	EXPECT_NO_THROW(validate_autenticode_timestamp_format(signature, errors));

	expect_contains_errors(errors,
		authenticode_timestamp_signature_format_validator_errc::invalid_tst_info_accuracy_value);
}

TYPED_TEST(AuthenticodeTimestampSignatureFormatValidatorTest, InvalidTstAccuracyMicros)
{
	using range_type = typename TestFixture::range_type;
	auto signature = create_signature<
		authenticode_signature_cms_info_ms_bug_workaround_type<range_type>>();

	add_signer_info_and_algorithm(signature);
	add_tst_oid(signature);
	add_tst_version(signature);
	add_tst_accuracy(signature, 100, -1);

	error_list errors;
	EXPECT_NO_THROW(validate_autenticode_timestamp_format(signature, errors));

	expect_contains_errors(errors,
		authenticode_timestamp_signature_format_validator_errc::invalid_tst_info_accuracy_value);
}

TYPED_TEST(AuthenticodeTimestampSignatureFormatValidatorTest, GenericSignatureCheckEmpty)
{
	using range_type = typename TestFixture::range_type;
	authenticode_timestamp_signature<range_type> signature{};
	error_list errors;
	EXPECT_NO_THROW(validate_autenticode_timestamp_format(signature, errors));

	expect_contains_errors(errors,
		pkcs7_format_validator_errc::absent_message_digest,
		pkcs7_format_validator_errc::absent_content_type);
}

TYPED_TEST(AuthenticodeTimestampSignatureFormatValidatorTest, GenericSignatureCheckValidPkcs7SignerInfo)
{
	using range_type = typename TestFixture::range_type;
	authenticode_timestamp_signature<range_type> signature{};
	auto& signer_info = std::get<signer_info_pkcs7<range_type>>(
		signature.get_underlying_type());

	auto& authenticated_attributes = signer_info.get_underlying().authenticated_attributes.emplace();
	authenticated_attributes.value.push_back({
		.type = {
			.container = this->oid_message_digest
		},
		.values = this->test_data
	});
	authenticated_attributes.value.push_back({
		.type = {
			.container = this->oid_content_type
		},
		.values = this->valid_oid_data
		});

	error_list errors;
	EXPECT_NO_THROW(validate_autenticode_timestamp_format(signature, errors));

	expect_contains_errors(errors);
}

TYPED_TEST(AuthenticodeTimestampSignatureFormatValidatorTest, GenericSignatureCheckValidCmsAndTst)
{
	using range_type = typename TestFixture::range_type;
	authenticode_timestamp_signature<range_type> signature{};
	auto& underlying = signature.get_underlying_type()
		.template emplace<authenticode_signature_cms_info_type<range_type>>();

	underlying = create_signature<authenticode_signature_cms_info_type<range_type>>();
	add_signer_info_and_algorithm(underlying);
	add_tst_oid(underlying);
	add_tst_version(underlying);
	add_tst_accuracy(underlying);

	error_list errors;
	EXPECT_NO_THROW(validate_autenticode_timestamp_format(signature, errors));

	expect_contains_errors(errors);
}
