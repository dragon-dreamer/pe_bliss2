#include "pe_bliss2/security/authenticode_verifier.h"

#include <variant>

#include "gtest/gtest.h"

#include "pe_bliss2/security/authenticode_pkcs7.h"
#include "pe_bliss2/image/image.h"

#include "tests/pe_bliss2/directories/security/hex_string_helpers.h"
#include "tests/pe_bliss2/pe_error_helper.h"

#include "simple_asn1/crypto/algorithms.h"
#include "simple_asn1/crypto/pkcs7/authenticode/oids.h"
#include "simple_asn1/crypto/pkcs7/authenticode/types.h"
#include "simple_asn1/crypto/pkcs7/oids.h"
#include "simple_asn1/crypto/pkcs9/oids.h"

#include "tests/pe_bliss2/directories/security/common_authenticode_data.h"

using namespace pe_bliss;
using namespace pe_bliss::security;

namespace
{
template<typename RangeType>
class AuthenticodeVerifierTest : public testing::Test
{
public:
	using range_type = RangeType;

	void add_signed_data_oid()
	{
		signature.get_content_info().content_type.container.assign(
			asn1::crypto::pkcs7::oid_signed_data.begin(),
			asn1::crypto::pkcs7::oid_signed_data.end());
	}

	void add_signed_data_version()
	{
		signature.get_content_info().data.version = 1u;
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

		auto& signed_data_content_info = signature.get_content_info().data.content_info;
		signed_data_content_info.content.digest.value.digest_algorithm.algorithm.container.assign(
			asn1::crypto::hash::id_sha256.begin(),
			asn1::crypto::hash::id_sha256.end());
	}

	void add_ms_oids()
	{
		auto& signed_data_content_info = signature.get_content_info().data.content_info;
		signed_data_content_info.content_type.container.assign(
			asn1::crypto::pkcs7::authenticode::oid_spc_indirect_data_content.begin(),
			asn1::crypto::pkcs7::authenticode::oid_spc_indirect_data_content.end());

		auto& type_value = signed_data_content_info.content.type_value.value;
		type_value.type.container.assign(
			asn1::crypto::pkcs7::authenticode::oid_spc_pe_image_data.begin(),
			asn1::crypto::pkcs7::authenticode::oid_spc_pe_image_data.end());
	}

	void init_authenticated_attributes(const std::vector<RangeType>& message_digest,
		bool with_signing_time = false)
	{
		auto& authenticated_attributes = signature.get_content_info().data.signer_infos
			.at(0).authenticated_attributes.emplace().value;

		authenticated_attributes.emplace_back(
			asn1::crypto::pkcs7::attribute<RangeType>{
			.type = asn1::decoded_object_identifier<std::vector<std::uint32_t>>{
				.container = oid_message_digest
			},
			.values = message_digest
		});

		authenticated_attributes.emplace_back(
			asn1::crypto::pkcs7::attribute<RangeType>{
			.type = asn1::decoded_object_identifier<std::vector<std::uint32_t>>{
				.container = oid_content_type
			},
			.values = this->valid_oid_data
		});

		if (with_signing_time)
		{
			authenticated_attributes.emplace_back(
				asn1::crypto::pkcs7::attribute<RangeType>{
				.type = asn1::decoded_object_identifier<std::vector<std::uint32_t>>{
					.container = oid_signing_time
				},
				.values = this->signing_time_data
			});
		}
	}

	void init_image()
	{
		image_instance.get_dos_header().get_descriptor()->e_lfanew = 0x10u;
		image_instance.get_data_directories().set_size(16u);
		image_instance.get_data_directories().get_directory(
			pe_bliss::core::data_directories::directory_type::security)->virtual_address = 1u;

		static constexpr std::size_t checksum_offset = 104u;
		static constexpr std::size_t cert_table_entry_offset = 168u;

		auto& headers = image_instance.get_full_headers_buffer().copied_data();
		headers.resize(512u);
		headers[1u] = std::byte{ 0x10u };
		headers[checksum_offset + 1] = std::byte{ 0x10u };
		headers[cert_table_entry_offset + 2] = std::byte{ 0x10u };
		headers[200u] = std::byte{ 0x10u };
	}

	void add_image_hash_to_signature()
	{
		signature.get_content_info()
			.data.content_info.content.digest.value.digest = image_hash;
	}

	void add_page_hashes_to_signature(bool with_valid_hash)
	{
		auto& file = signature.get_content_info()
			.data.content_info.content.type_value.value.value.file.emplace();
		auto& spc_ser_obj = file.template emplace<
			asn1::crypto::pkcs7::authenticode::spc_serialized_object<range_type>>();
		spc_ser_obj.class_id = page_hashes_class_id;

		if (with_valid_hash)
			spc_ser_obj.serialized_data = page_hashes_asn1_der;
	}

	void add_message_digest_to_signer()
	{
		signature.get_content_info()
			.data.content_info.content.digest.raw = unhashed_message_digest;
	}

	void add_certificate_to_store()
	{
		auto& certificates = signature.get_content_info().data.certificates.emplace();
		auto& cert = certificates.emplace_back().template emplace<0>();
		cert.tbs_cert.serial_number = serial_number;
		cert.tbs_cert.issuer.raw = raw_issuer;
	}

	void add_issuer_and_sn()
	{
		auto& signer_info = signature.get_content_info().data.signer_infos.back();
		auto& issuer_and_sn = signer_info.issuer_and_sn;
		issuer_and_sn.issuer.raw = raw_issuer;
		issuer_and_sn.serial_number = serial_number;
	}

	void set_rsa1_signer_data(bool valid_hash)
	{
		auto& signer_info = signature.get_content_info().data.signer_infos.at(0);
		auto& certificate_in_store = std::get<0>(signature.get_content_info()
			.data.certificates.value().at(0));
		certificate_in_store.tbs_cert.pki.subject_publickey.container = valid_rsa1_public_key;
		signer_info.encrypted_digest = valid_rsa1_encrypted_digest;
		signer_info.authenticated_attributes.value().raw = valid_hash
			? valid_raw_authenticated_attributes
			: invalid_raw_authenticated_attributes;
	}

	template<typename SignatureCheckResult>
	static void check_signing_time(const SignatureCheckResult& result)
	{
		ASSERT_TRUE(result.signing_time);
		const auto& signing_time = result.signing_time.value();
		ASSERT_EQ(signing_time.year, 96u);
		ASSERT_EQ(signing_time.month, 4u);
		ASSERT_EQ(signing_time.day, 15u);
		ASSERT_EQ(signing_time.hour, 20u);
		ASSERT_EQ(signing_time.minute, 30u);
		ASSERT_EQ(signing_time.second, 1u);
	}

	void add_nested_authenticode()
	{
		static const std::vector<std::byte> valid_authenticode_copy{
			reinterpret_cast<const std::byte*>(valid_authenticode.data()),
			reinterpret_cast<const std::byte*>(valid_authenticode.data())
				+ valid_authenticode.size()
		};
		static const std::vector<range_type> attr_data{
			valid_authenticode_copy, valid_authenticode_copy };

		auto& unauthenticated_attributes = signature.get_content_info().data.signer_infos
			.at(0).unauthenticated_attributes.emplace();

		unauthenticated_attributes.emplace_back(
			asn1::crypto::pkcs7::attribute<RangeType>{
			.type = asn1::decoded_object_identifier<std::vector<std::uint32_t>>{
				.container = oid_nested_signature_attribute
			},
			.values = attr_data
		});
	}

public:
	authenticode_pkcs7<range_type> signature;
	pe_bliss::image::image image_instance;

	static const inline std::vector valid_oid{
		hex_string_to_bytes("060a2b060104018237020104")};
	static const inline std::vector<RangeType> valid_oid_data{ valid_oid };

	static const inline std::vector<std::uint32_t> oid_message_digest{
		asn1::crypto::pkcs7::oid_message_digest.cbegin(),
		asn1::crypto::pkcs7::oid_message_digest.cend() };
	static const inline std::vector<std::uint32_t> oid_content_type{
		asn1::crypto::pkcs7::oid_content_type.cbegin(),
		asn1::crypto::pkcs7::oid_content_type.cend() };
	static const inline std::vector<std::uint32_t> oid_signing_time{
		asn1::crypto::pkcs7::oid_signing_time.cbegin(),
		asn1::crypto::pkcs7::oid_signing_time.cend() };
	static const inline std::vector<std::uint32_t> oid_nested_signature_attribute{
		asn1::crypto::pkcs7::authenticode::oid_nested_signature_attribute.cbegin(),
		asn1::crypto::pkcs7::authenticode::oid_nested_signature_attribute.cend() };
	static const inline std::vector<std::byte> page_hashes_class_id{
		asn1::crypto::pkcs7::authenticode::page_hashes_class_id.cbegin(),
		asn1::crypto::pkcs7::authenticode::page_hashes_class_id.cend() };

	static const inline std::vector<std::byte> valid_utc_time{
		std::byte{0x17u}, std::byte{0x0du},
		std::byte{0x39u}, std::byte{0x36u}, std::byte{0x30u}, std::byte{0x34u},
		std::byte{0x31u}, std::byte{0x35u}, std::byte{0x32u}, std::byte{0x30u},
		std::byte{0x33u}, std::byte{0x30u}, std::byte{0x30u}, std::byte{0x31u},
		std::byte{0x5au} };
	static const inline std::vector<RangeType> signing_time_data{ valid_utc_time };

	static const inline std::vector valid_message_digest{
		std::byte{0x04u}, std::byte{0x02u},
		std::byte{1}, std::byte{2} };
	static const inline std::vector<RangeType> valid_message_digest_data{
		valid_message_digest };
	// sha256 of "ab" string
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

	static const inline std::vector unhashed_message_digest{
		std::byte{'a'}, std::byte{'b'}
	};

	static const inline std::vector image_hash{
		hex_string_to_bytes("d27e0af9a41c1a977dc73024a167f1fcc56811fbe6a29"
			"dd3e761412970f03397")
	};
	static const inline std::vector page_hashes{
		hex_string_to_bytes("00000000a6a9f2ce4927474daee2b3607e357718e93b1"
			"e48bb8b70bf1ce508d4b35a80370002000000000000000000000000000000"
			"00000000000000000000000000000000000000")
	};
	static const inline std::vector page_hashes_asn1_der{
		hex_string_to_bytes("315a3058060a2b060104018237020302"
			"314a044800000000a6a9f2ce4927474daee2b3607e357718"
			"e93b1e48bb8b70bf1ce508d4b35a80370002000000000000"
			"00000000000000000000000000000000000000000000000000000000")
	};

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

using tested_range_types = ::testing::Types<
	vector_range_type, span_range_type>;
} //namespace

TYPED_TEST_SUITE(AuthenticodeVerifierTest, tested_range_types);

TYPED_TEST(AuthenticodeVerifierTest, Empty)
{
	auto result = verify_authenticode_full(this->signature, this->image_instance);
	ASSERT_FALSE(result);
	expect_contains_errors(result.root.authenticode_format_errors,
		pkcs7::pkcs7_format_validator_errc::invalid_signed_data_oid,
		pkcs7::pkcs7_format_validator_errc::invalid_signed_data_version,
		pkcs7::pkcs7_format_validator_errc::invalid_signer_count);
	expect_contains_errors(result.root.certificate_store_warnings);
	ASSERT_FALSE(result.root.image_hash_valid);
	ASSERT_FALSE(result.root.page_hashes_valid);
	ASSERT_FALSE(result.root.page_hashes_check_errc);
	ASSERT_FALSE(result.root.message_digest_valid);
	ASSERT_FALSE(result.root.signature_result);
	ASSERT_FALSE(result.root.image_digest_alg);
	ASSERT_FALSE(result.root.digest_encryption_alg);
	ASSERT_FALSE(result.root.signing_time);
	ASSERT_FALSE(result.root.cert_store);
	ASSERT_FALSE(result.root.timestamp_signature_result);
	ASSERT_TRUE(result.nested.empty());
}

TYPED_TEST(AuthenticodeVerifierTest, ValidFormat)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_ms_oids();

	auto result = verify_authenticode_full(this->signature, this->image_instance);

	ASSERT_FALSE(result);
	expect_contains_errors(result.root.authenticode_format_errors,
		pkcs7::pkcs7_format_validator_errc::absent_message_digest,
		pkcs7::pkcs7_format_validator_errc::absent_content_type);
	expect_contains_errors(result.root.certificate_store_warnings);
	ASSERT_FALSE(result.root.image_hash_valid);
	ASSERT_FALSE(result.root.page_hashes_valid);
	ASSERT_FALSE(result.root.page_hashes_check_errc);
	ASSERT_FALSE(result.root.message_digest_valid);
	ASSERT_FALSE(result.root.signature_result);
	ASSERT_FALSE(result.root.image_digest_alg);
	ASSERT_FALSE(result.root.digest_encryption_alg);
	ASSERT_FALSE(result.root.signing_time);
	ASSERT_FALSE(result.root.cert_store);
	ASSERT_FALSE(result.root.timestamp_signature_result);
	ASSERT_TRUE(result.nested.empty());
}

TYPED_TEST(AuthenticodeVerifierTest, WithAuthenticatedAttributes)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_ms_oids();
	this->init_authenticated_attributes(this->valid_message_digest_data);

	auto result = verify_authenticode_full(this->signature, this->image_instance);

	ASSERT_FALSE(result);
	expect_contains_errors(result.root.authenticode_format_errors,
		authenticode_verifier_errc::invalid_image_format_for_hashing);
	expect_contains_errors(result.root.certificate_store_warnings,
		certificate_store_errc::absent_certificates);
	ASSERT_FALSE(result.root.image_hash_valid);
	ASSERT_FALSE(result.root.page_hashes_valid);
	ASSERT_FALSE(result.root.page_hashes_check_errc);
	ASSERT_FALSE(result.root.message_digest_valid);
	ASSERT_FALSE(result.root.signature_result);
	ASSERT_EQ(result.root.image_digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.root.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_FALSE(result.root.signing_time);
	ASSERT_TRUE(result.root.cert_store);
	ASSERT_TRUE(result.root.cert_store->empty());
	ASSERT_FALSE(result.root.timestamp_signature_result);
	ASSERT_TRUE(result.nested.empty());
}

TYPED_TEST(AuthenticodeVerifierTest, WithValidImage)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_ms_oids();
	this->init_authenticated_attributes(this->valid_message_digest_data);
	this->init_image();

	auto result = verify_authenticode_full(this->signature, this->image_instance);

	ASSERT_FALSE(result);
	expect_contains_errors(result.root.authenticode_format_errors);
	expect_contains_errors(result.root.certificate_store_warnings,
		certificate_store_errc::absent_certificates);
	ASSERT_TRUE(result.root.image_hash_valid);
	ASSERT_FALSE(*result.root.image_hash_valid);
	ASSERT_FALSE(result.root.page_hashes_valid);
	ASSERT_FALSE(result.root.page_hashes_check_errc);
	ASSERT_TRUE(result.root.message_digest_valid);
	ASSERT_FALSE(*result.root.message_digest_valid);
	ASSERT_TRUE(result.root.signature_result);
	ASSERT_FALSE(*result.root.signature_result);
	ASSERT_EQ(result.root.image_digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.root.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_FALSE(result.root.signing_time);
	ASSERT_TRUE(result.root.cert_store);
	ASSERT_TRUE(result.root.cert_store->empty());
	ASSERT_FALSE(result.root.timestamp_signature_result);
	ASSERT_TRUE(result.nested.empty());
}

TYPED_TEST(AuthenticodeVerifierTest, WithValidImageAndInvalidPageHashes)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_ms_oids();
	this->init_authenticated_attributes(this->valid_message_digest_data);
	this->init_image();
	this->add_image_hash_to_signature();
	this->add_page_hashes_to_signature(false);

	auto result = verify_authenticode_full(this->signature, this->image_instance);

	ASSERT_FALSE(result);
	expect_contains_errors(result.root.authenticode_format_errors);
	expect_contains_errors(result.root.certificate_store_warnings,
		certificate_store_errc::absent_certificates);
	ASSERT_TRUE(result.root.image_hash_valid);
	ASSERT_TRUE(*result.root.image_hash_valid);
	ASSERT_FALSE(result.root.page_hashes_valid);
	ASSERT_EQ(result.root.page_hashes_check_errc,
		authenticode_verifier_errc::invalid_page_hash_format);
	ASSERT_TRUE(result.root.message_digest_valid);
	ASSERT_FALSE(*result.root.message_digest_valid);
	ASSERT_TRUE(result.root.signature_result);
	ASSERT_FALSE(*result.root.signature_result);
	ASSERT_EQ(result.root.image_digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.root.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_FALSE(result.root.signing_time);
	ASSERT_TRUE(result.root.cert_store);
	ASSERT_TRUE(result.root.cert_store->empty());
	ASSERT_FALSE(result.root.timestamp_signature_result);
	ASSERT_TRUE(result.nested.empty());
}

TYPED_TEST(AuthenticodeVerifierTest, WithValidImageAndPageHashes)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_ms_oids();
	this->init_authenticated_attributes(this->valid_message_digest_data);
	this->init_image();
	this->add_image_hash_to_signature();
	this->add_page_hashes_to_signature(true);

	auto result = verify_authenticode_full(this->signature, this->image_instance);

	ASSERT_FALSE(result);
	expect_contains_errors(result.root.authenticode_format_errors);
	expect_contains_errors(result.root.certificate_store_warnings,
		certificate_store_errc::absent_certificates);
	ASSERT_TRUE(result.root.image_hash_valid);
	ASSERT_TRUE(*result.root.image_hash_valid);
	ASSERT_TRUE(result.root.page_hashes_valid);
	ASSERT_TRUE(*result.root.page_hashes_valid);
	ASSERT_FALSE(result.root.page_hashes_check_errc);
	ASSERT_TRUE(result.root.message_digest_valid);
	ASSERT_FALSE(*result.root.message_digest_valid);
	ASSERT_TRUE(result.root.signature_result);
	ASSERT_FALSE(*result.root.signature_result);
	ASSERT_EQ(result.root.image_digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.root.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_FALSE(result.root.signing_time);
	ASSERT_TRUE(result.root.cert_store);
	ASSERT_TRUE(result.root.cert_store->empty());
	ASSERT_FALSE(result.root.timestamp_signature_result);
	ASSERT_TRUE(result.nested.empty());
}

TYPED_TEST(AuthenticodeVerifierTest, WithIncorrectMessageDigest)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_ms_oids();
	this->init_authenticated_attributes(this->valid_message_digest_data);
	this->init_image();
	this->add_image_hash_to_signature();
	this->add_page_hashes_to_signature(true);
	this->add_message_digest_to_signer();

	auto result = verify_authenticode_full(this->signature, this->image_instance);

	ASSERT_FALSE(result);
	expect_contains_errors(result.root.authenticode_format_errors);
	expect_contains_errors(result.root.certificate_store_warnings,
		certificate_store_errc::absent_certificates);
	ASSERT_TRUE(result.root.image_hash_valid);
	ASSERT_TRUE(*result.root.image_hash_valid);
	ASSERT_TRUE(result.root.page_hashes_valid);
	ASSERT_TRUE(*result.root.page_hashes_valid);
	ASSERT_FALSE(result.root.page_hashes_check_errc);
	ASSERT_TRUE(result.root.message_digest_valid);
	ASSERT_FALSE(*result.root.message_digest_valid);
	ASSERT_TRUE(result.root.signature_result);
	ASSERT_FALSE(*result.root.signature_result);
	ASSERT_EQ(result.root.image_digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.root.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_FALSE(result.root.signing_time);
	ASSERT_TRUE(result.root.cert_store);
	ASSERT_TRUE(result.root.cert_store->empty());
	ASSERT_FALSE(result.root.timestamp_signature_result);
	ASSERT_TRUE(result.nested.empty());
}

TYPED_TEST(AuthenticodeVerifierTest, WithCorrectMessageDigest)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_ms_oids();
	this->init_authenticated_attributes(this->valid_and_correct_message_digest_data);
	this->init_image();
	this->add_image_hash_to_signature();
	this->add_page_hashes_to_signature(true);
	this->add_message_digest_to_signer();

	auto result = verify_authenticode_full(this->signature, this->image_instance);

	ASSERT_FALSE(result);
	expect_contains_errors(result.root.authenticode_format_errors);
	expect_contains_errors(result.root.certificate_store_warnings,
		certificate_store_errc::absent_certificates);
	ASSERT_TRUE(result.root.image_hash_valid);
	ASSERT_TRUE(*result.root.image_hash_valid);
	ASSERT_TRUE(result.root.page_hashes_valid);
	ASSERT_TRUE(*result.root.page_hashes_valid);
	ASSERT_FALSE(result.root.page_hashes_check_errc);
	ASSERT_TRUE(result.root.message_digest_valid);
	ASSERT_TRUE(*result.root.message_digest_valid);
	ASSERT_TRUE(result.root.signature_result);
	ASSERT_FALSE(*result.root.signature_result);
	ASSERT_EQ(result.root.image_digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.root.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_FALSE(result.root.signing_time);
	ASSERT_TRUE(result.root.cert_store);
	ASSERT_TRUE(result.root.cert_store->empty());
	ASSERT_FALSE(result.root.timestamp_signature_result);
	ASSERT_TRUE(result.nested.empty());
}

TYPED_TEST(AuthenticodeVerifierTest, WithSigningTime)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_ms_oids();
	this->init_authenticated_attributes(this->valid_and_correct_message_digest_data, true);
	this->init_image();
	this->add_image_hash_to_signature();
	this->add_page_hashes_to_signature(true);
	this->add_message_digest_to_signer();

	auto result = verify_authenticode_full(this->signature, this->image_instance);

	ASSERT_FALSE(result);
	expect_contains_errors(result.root.authenticode_format_errors);
	expect_contains_errors(result.root.certificate_store_warnings,
		certificate_store_errc::absent_certificates);
	ASSERT_TRUE(result.root.image_hash_valid);
	ASSERT_TRUE(*result.root.image_hash_valid);
	ASSERT_TRUE(result.root.page_hashes_valid);
	ASSERT_TRUE(*result.root.page_hashes_valid);
	ASSERT_FALSE(result.root.page_hashes_check_errc);
	ASSERT_TRUE(result.root.message_digest_valid);
	ASSERT_TRUE(*result.root.message_digest_valid);
	ASSERT_TRUE(result.root.signature_result);
	ASSERT_FALSE(*result.root.signature_result);
	ASSERT_EQ(result.root.image_digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.root.digest_encryption_alg, digest_encryption_algorithm::rsa);
	this->check_signing_time(result.root);
	ASSERT_TRUE(result.root.cert_store);
	ASSERT_TRUE(result.root.cert_store->empty());
	ASSERT_FALSE(result.root.timestamp_signature_result);
	ASSERT_TRUE(result.nested.empty());
}

TYPED_TEST(AuthenticodeVerifierTest, WithCertificate)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_ms_oids();
	this->init_authenticated_attributes(this->valid_and_correct_message_digest_data, true);
	this->init_image();
	this->add_image_hash_to_signature();
	this->add_page_hashes_to_signature(true);
	this->add_message_digest_to_signer();
	this->add_certificate_to_store();

	auto result = verify_authenticode_full(this->signature, this->image_instance);

	ASSERT_FALSE(result);
	expect_contains_errors(result.root.authenticode_format_errors);
	expect_contains_errors(result.root.certificate_store_warnings);
	ASSERT_TRUE(result.root.image_hash_valid);
	ASSERT_TRUE(*result.root.image_hash_valid);
	ASSERT_TRUE(result.root.page_hashes_valid);
	ASSERT_TRUE(*result.root.page_hashes_valid);
	ASSERT_FALSE(result.root.page_hashes_check_errc);
	ASSERT_TRUE(result.root.message_digest_valid);
	ASSERT_TRUE(*result.root.message_digest_valid);
	ASSERT_TRUE(result.root.signature_result);
	ASSERT_FALSE(*result.root.signature_result);
	ASSERT_EQ(result.root.image_digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.root.digest_encryption_alg, digest_encryption_algorithm::rsa);
	this->check_signing_time(result.root);
	ASSERT_TRUE(result.root.cert_store);
	ASSERT_EQ(result.root.cert_store->size(), 1u);
	ASSERT_FALSE(result.root.timestamp_signature_result);
	ASSERT_TRUE(result.nested.empty());
}

TYPED_TEST(AuthenticodeVerifierTest, WithInvalidSignature)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_ms_oids();
	this->init_authenticated_attributes(this->valid_and_correct_message_digest_data, true);
	this->init_image();
	this->add_image_hash_to_signature();
	this->add_page_hashes_to_signature(true);
	this->add_message_digest_to_signer();
	this->add_certificate_to_store();
	this->add_issuer_and_sn();
	this->set_rsa1_signer_data(false);

	auto result = verify_authenticode_full(this->signature, this->image_instance);

	ASSERT_FALSE(result);
	expect_contains_errors(result.root.authenticode_format_errors);
	expect_contains_errors(result.root.certificate_store_warnings);
	ASSERT_TRUE(result.root.image_hash_valid);
	ASSERT_TRUE(*result.root.image_hash_valid);
	ASSERT_TRUE(result.root.page_hashes_valid);
	ASSERT_TRUE(*result.root.page_hashes_valid);
	ASSERT_FALSE(result.root.page_hashes_check_errc);
	ASSERT_TRUE(result.root.message_digest_valid);
	ASSERT_TRUE(*result.root.message_digest_valid);
	ASSERT_TRUE(result.root.signature_result);
	ASSERT_FALSE(*result.root.signature_result);
	ASSERT_EQ(result.root.image_digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.root.digest_encryption_alg, digest_encryption_algorithm::rsa);
	this->check_signing_time(result.root);
	ASSERT_TRUE(result.root.cert_store);
	ASSERT_EQ(result.root.cert_store->size(), 1u);
	ASSERT_FALSE(result.root.timestamp_signature_result);
	ASSERT_TRUE(result.nested.empty());
}

TYPED_TEST(AuthenticodeVerifierTest, WithValidSignature)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_ms_oids();
	this->init_authenticated_attributes(this->valid_and_correct_message_digest_data, true);
	this->init_image();
	this->add_image_hash_to_signature();
	this->add_page_hashes_to_signature(true);
	this->add_message_digest_to_signer();
	this->add_certificate_to_store();
	this->add_issuer_and_sn();
	this->set_rsa1_signer_data(true);

	auto result = verify_authenticode_full(this->signature, this->image_instance);

	ASSERT_TRUE(result);
	expect_contains_errors(result.root.authenticode_format_errors);
	expect_contains_errors(result.root.certificate_store_warnings);
	ASSERT_TRUE(result.root.image_hash_valid);
	ASSERT_TRUE(*result.root.image_hash_valid);
	ASSERT_TRUE(result.root.page_hashes_valid);
	ASSERT_TRUE(*result.root.page_hashes_valid);
	ASSERT_FALSE(result.root.page_hashes_check_errc);
	ASSERT_TRUE(result.root.message_digest_valid);
	ASSERT_TRUE(*result.root.message_digest_valid);
	ASSERT_TRUE(result.root.signature_result);
	ASSERT_TRUE(*result.root.signature_result);
	ASSERT_EQ(result.root.image_digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.root.digest_encryption_alg, digest_encryption_algorithm::rsa);
	this->check_signing_time(result.root);
	ASSERT_TRUE(result.root.cert_store);
	ASSERT_EQ(result.root.cert_store->size(), 1u);
	ASSERT_FALSE(result.root.timestamp_signature_result);
	ASSERT_TRUE(result.nested.empty());
}

TYPED_TEST(AuthenticodeVerifierTest, WithNestedSignature)
{
	this->add_signed_data_oid();
	this->add_signed_data_version();
	this->add_signer_info_and_algorithm();
	this->add_ms_oids();
	this->init_authenticated_attributes(this->valid_and_correct_message_digest_data, true);
	this->init_image();
	this->add_image_hash_to_signature();
	this->add_page_hashes_to_signature(true);
	this->add_message_digest_to_signer();
	this->add_certificate_to_store();
	this->add_issuer_and_sn();
	this->set_rsa1_signer_data(true);
	this->add_nested_authenticode();

	auto result = verify_authenticode_full(this->signature, this->image_instance);

	ASSERT_FALSE(result);
	expect_contains_errors(result.root.authenticode_format_errors);
	expect_contains_errors(result.root.certificate_store_warnings);
	ASSERT_TRUE(result.root.image_hash_valid);
	ASSERT_TRUE(*result.root.image_hash_valid);
	ASSERT_TRUE(result.root.page_hashes_valid);
	ASSERT_TRUE(*result.root.page_hashes_valid);
	ASSERT_FALSE(result.root.page_hashes_check_errc);
	ASSERT_TRUE(result.root.message_digest_valid);
	ASSERT_TRUE(*result.root.message_digest_valid);
	ASSERT_TRUE(result.root.signature_result);
	ASSERT_TRUE(*result.root.signature_result);
	ASSERT_EQ(result.root.image_digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(result.root.digest_encryption_alg, digest_encryption_algorithm::rsa);
	this->check_signing_time(result.root);
	ASSERT_TRUE(result.root.cert_store);
	ASSERT_EQ(result.root.cert_store->size(), 1u);
	ASSERT_FALSE(result.root.timestamp_signature_result);

	ASSERT_EQ(result.nested.size(), 2u);

	const auto& nested0 = result.nested.at(0);
	ASSERT_FALSE(nested0);
	expect_contains_errors(nested0.authenticode_format_errors);
	expect_contains_errors(nested0.certificate_store_warnings);
	ASSERT_TRUE(nested0.image_hash_valid);
	ASSERT_FALSE(*nested0.image_hash_valid);
	ASSERT_FALSE(nested0.page_hashes_valid);
	ASSERT_FALSE(nested0.page_hashes_check_errc);
	ASSERT_TRUE(nested0.message_digest_valid);
	ASSERT_TRUE(*nested0.message_digest_valid);
	ASSERT_TRUE(nested0.signature_result);
	ASSERT_TRUE(*nested0.signature_result);
	ASSERT_EQ(nested0.image_digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(nested0.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_FALSE(nested0.signing_time);
	ASSERT_TRUE(nested0.cert_store);
	ASSERT_EQ(nested0.cert_store->size(), 4u);

	ASSERT_TRUE(nested0.timestamp_signature_result);
	const auto& nested0_ts = nested0.timestamp_signature_result.value();
	expect_contains_errors(nested0_ts.authenticode_format_errors);
	expect_contains_errors(nested0_ts.certificate_store_warnings);
	ASSERT_TRUE(nested0_ts.hash_valid);
	ASSERT_TRUE(*nested0_ts.hash_valid);
	ASSERT_FALSE(nested0_ts.message_digest_valid);
	ASSERT_FALSE(nested0_ts.imprint_digest_alg);
	ASSERT_TRUE(nested0_ts.signature_result);
	ASSERT_TRUE(*nested0_ts.signature_result);
	ASSERT_EQ(nested0_ts.digest_alg, digest_algorithm::sha256);
	ASSERT_EQ(nested0_ts.digest_encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_FALSE(nested0_ts.cert_store);
	const auto* ts0_signing_time = std::get_if<1>(&nested0_ts.signing_time);
	ASSERT_NE(ts0_signing_time, nullptr);
	ASSERT_EQ(ts0_signing_time->year, 0x17u);
	ASSERT_EQ(ts0_signing_time->minute, 0x1du);
}