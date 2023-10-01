#include "pe_bliss2/security/pkcs7/pkcs7_signature.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include "gtest/gtest.h"

#include "pe_bliss2/pe_error.h"

#include "tests/pe_bliss2/directories/security/hex_string_helpers.h"

using namespace pe_bliss::security;
using namespace pe_bliss::security::pkcs7;

TEST(Pkcs7SignatureTests, EmptyData)
{
	ASSERT_THROW((void)verify_signature({}, {}, {},
		digest_algorithm::md5,
		digest_encryption_algorithm::unknown, {}), pe_bliss::pe_error);
}

TEST(Pkcs7SignatureTests, UnknownSignatureAlgorithm)
{
	const std::vector<std::byte> data{ std::byte{1} };
	ASSERT_THROW((void)verify_signature(data, data, data,
		digest_algorithm::md5,
		digest_encryption_algorithm::unknown, {}), pe_bliss::pe_error);
}

TEST(Pkcs7SignatureTests, UnknownHashAlgorithm)
{
	const std::vector<std::byte> data{ std::byte{1} };
	ASSERT_THROW((void)verify_signature(data, data, data,
		digest_algorithm::unknown,
		digest_encryption_algorithm::rsa, {}), pe_bliss::pe_error);
	ASSERT_THROW((void)verify_signature(data, data, data,
		digest_algorithm::unknown,
		digest_encryption_algorithm::ecdsa, {}), pe_bliss::pe_error);
	ASSERT_THROW((void)verify_signature(data, data, data,
		digest_algorithm::unknown,
		digest_encryption_algorithm::ecdsa, data), pe_bliss::pe_error);
}

TEST(Pkcs7SignatureTests, RsaSha256Valid)
{
	const auto pk = hex_string_to_bytes("30818902818100d184617b5f8034655944839f785a63835555088a"
		"23d0b34e1a2e6bdf83c49ba2b1ecb398105eed1a21d513ea76f9ad3879843db27e91765885ba33ccf45b14"
		"61c227205f08bcd07d5a2cf7fa9443cf2ef376f448503630699059002546d2f2eba124478ac34704e3d83c"
		"d1e041178042a922fa3c541b3fdfeb072c5dc44a00210203010001");

	const auto message_digest = hex_string_to_bytes(
		"9834876dcfb05cb167a5c24953eba58c4ac89b1adf57f28f2f9d09af107ee8f0");

	const auto encrypted_digest = hex_string_to_bytes("c27ebd5c0193244f7d3f08405b6821c2085ee522"
		"c630aad3e1a14e4ae2dee034f47e480fb37a729e074f98fb8b70dd387666b4b9def14c3b5e2a814cf13bc6"
		"fca24f252f396fdcf0aca55c0ca7782e9d07ca887714761ee0bdb8364a1816c2e43b0ceef5a507b8a939d8"
		"bbad1f38ee5dfdd30f04b21eeeb406b9d2d5478fac87");

	ASSERT_EQ(verify_signature(pk, message_digest, encrypted_digest,
		digest_algorithm::sha256,
		digest_encryption_algorithm::rsa, {}), (signature_verification_result{ true, 1024u }));
}

TEST(Pkcs7SignatureTests, RsaSha256Invalid)
{
	const auto pk = hex_string_to_bytes("30818902818100d184617b5f8034655944839f785a63835555088a"
		"23d0b34e1a2e6bdf83c49ba2b1ecb398105eed1a21d513ea76f9ad3879843db27e91765885ba33ccf45b14"
		"61c227205f08bcd07d5a2cf7fa9443cf2ef376f448503630699059002546d2f2eba124478ac34704e3d83c"
		"d1e041178042a922fa3c541b3fdfeb072c5dc44a00210203010001");

	const auto message_digest = hex_string_to_bytes(
		"9834876dcfb05cb167a5c24953eba58c4ac89b1adf57f28f2f9d09af107ee8f0");

	const auto encrypted_digest = hex_string_to_bytes("c27ebd5c0193244f7d3f08405b6821c2085ee522"
		"c630aad3e1a14e4ae2dee034f47e480fb37a729e074f98fb8b70dd387666b4b9def14c3b5e2a814cf13bc6"
		"fca24f252f396fdcf0aca55c0ca7782e9d07cb887714761ee0bdb8364a1816c2e43b0ceef5a507b8a939d8"
		"bbad1f38ee5dfdd30f04b21eeeb406b9d2d5478fac87");

	ASSERT_FALSE(verify_signature(pk, message_digest, encrypted_digest,
		digest_algorithm::sha256,
		digest_encryption_algorithm::rsa, {}));
}

TEST(Pkcs7SignatureTests, RsaSha1Valid)
{
	const auto pk = hex_string_to_bytes("30818902818100d184617b5f8034655944839f785a63835555088a"
		"23d0b34e1a2e6bdf83c49ba2b1ecb398105eed1a21d513ea76f9ad3879843db27e91765885ba33ccf45b14"
		"61c227205f08bcd07d5a2cf7fa9443cf2ef376f448503630699059002546d2f2eba124478ac34704e3d83c"
		"d1e041178042a922fa3c541b3fdfeb072c5dc44a00210203010001");

	const auto message_digest = hex_string_to_bytes(
		"7e240de74fb1ed08fa08d38063f6a6a91462a815");

	const auto encrypted_digest = hex_string_to_bytes(
		"6f7df91d8f973a0619d525c319337741130b77b21f9667dc7d1d74853b644cbe"
		"5e6b0e84aacc2faee883d43affb811fc653b67c38203d4f206d1b838c4714b6b"
		"2cf17cd621303c21bac96090df3883e58784a0576e501c10cdefb12b6bf887e5"
		"48f6b07b09ae80d8416151d7dab7066d645e2eee57ac5f7af2a70ee0724c8e47");

	ASSERT_EQ(verify_signature(pk, message_digest, encrypted_digest,
		digest_algorithm::sha1,
		digest_encryption_algorithm::rsa, {}), (signature_verification_result{ true, 1024u }));
}

TEST(Pkcs7SignatureTests, RsaSha1Invalid)
{
	const auto pk = hex_string_to_bytes("30818902818100d184617b5f8034655944839f785a63835555088a"
		"23d0b34e1a2e6bdf83c49ba2b1ecb398105eed1a21d513ea76f9ad3879843db27e91765885ba33ccf45b14"
		"61c227205f08bcd07d5a2cf7fa9443cf2ef376f448503630699059002546d2f2eba124478ac34704e3d83c"
		"d1e041178042a922fa3c541b3fdfeb072c5dc44a00210203010001");

	const auto message_digest = hex_string_to_bytes(
		"7e240de74fb1ed08fa08d38063f6a6a91462a8"); //too short hash

	const auto encrypted_digest = hex_string_to_bytes(
		"6f7df91d8f973a0619d525c319337741130b77b21f9667dc7d1d74853b644cbe"
		"5e6b0e84aacc2faee883d43affb811fc653b67c38203d4f206d1b838c4714b6b"
		"2cf17cd621303c21bac96090df3883e58784a0576e501c10cdefb12b6bf887e5"
		"48f6b07b09ae80d8416151d7dab7066d645e2eee57ac5f7af2a70ee0724c8e47");

	ASSERT_THROW((void)verify_signature(pk, message_digest, encrypted_digest,
		digest_algorithm::sha1,
		digest_encryption_algorithm::rsa, {}), pe_bliss::pe_error);
}

TEST(Pkcs7SignatureTests, RsaMd5Valid)
{
	const auto pk = hex_string_to_bytes("30818902818100d184617b5f8034655944839f785a63835555088a"
		"23d0b34e1a2e6bdf83c49ba2b1ecb398105eed1a21d513ea76f9ad3879843db27e91765885ba33ccf45b14"
		"61c227205f08bcd07d5a2cf7fa9443cf2ef376f448503630699059002546d2f2eba124478ac34704e3d83c"
		"d1e041178042a922fa3c541b3fdfeb072c5dc44a00210203010001");

	const auto message_digest = hex_string_to_bytes(
		"47bce5c74f589f4867dbd57e9ca9f808");

	const auto encrypted_digest = hex_string_to_bytes(
		"c22bdae3f670accdbd5f1a7f078cf0fe61077d59c795c9a206b09ad369bb61e1"
		"3a83f39934182612d8c8b9d4c4b35a628e16dc070f4323634258535a58c45c58"
		"f0fa068303b97d2cdabcbb4b658776b50df56557bce34fdbf54c151d763206fd"
		"2d7d1c56992197123cf53fa90fab2e339d0eca7b2783c6b7ebb88f026056bbd1");

	ASSERT_EQ(verify_signature(pk, message_digest, encrypted_digest,
		digest_algorithm::md5,
		digest_encryption_algorithm::rsa, {}), (signature_verification_result{ true, 1024u }));
}

TEST(Pkcs7SignatureTests, RsaMd5InvalidKey)
{
	const auto pk = hex_string_to_bytes("30838902818100d184617b5f8034655944839f785a63835555088a"
		"23d0b34e1a2e6bdf83c49ba2b1ecb398105eed1a21d513ea76f9ad3879843db27e91765885ba33ccf45b14"
		"61c227205f08bcd07d5a2cf7fa9443cf2ef376f448503630699059002546d2f2eba124478ac34704e3d83c"
		"d1e041178042a922fa3c541b3fdfeb072c5dc44a00210203010001"); //Invalid ASN.1

	const auto message_digest = hex_string_to_bytes(
		"47bce5c74f589f4867dbd57e9ca9f808");

	const auto encrypted_digest = hex_string_to_bytes(
		"c22bdae3f670accdbd5f1a7f078cf0fe61077d59c795c9a206b09ad369bb61e1"
		"3a83f39934182612d8c8b9d4c4b35a628e16dc070f4323634258535a58c45c58"
		"f0fa068303b97d2cdabcbb4b658776b50df56557bce34fdbf54c151d763206fd"
		"2d7d1c56992197123cf53fa90fab2e339d0eca7b2783c6b7ebb88f026056bbd1");

	ASSERT_THROW((void)verify_signature(pk, message_digest, encrypted_digest,
		digest_algorithm::md5,
		digest_encryption_algorithm::rsa, {}), pe_bliss::pe_error);
}

TEST(Pkcs7SignatureTests, RsaMd5ValidNoHashIdentity)
{
	const auto pk = hex_string_to_bytes("30818902818100d184617b5f8034655944839f785a63835555088a"
		"23d0b34e1a2e6bdf83c49ba2b1ecb398105eed1a21d513ea76f9ad3879843db27e91765885ba33ccf45b14"
		"61c227205f08bcd07d5a2cf7fa9443cf2ef376f448503630699059002546d2f2eba124478ac34704e3d83c"
		"d1e041178042a922fa3c541b3fdfeb072c5dc44a00210203010001");

	const auto message_digest = hex_string_to_bytes(
		"47bce5c74f589f4867dbd57e9ca9f808");

	//No MD5 hash identity here, just the signature octets
	const auto encrypted_digest = hex_string_to_bytes(
		"07da6b62949202c4e5807aac8ef83af611839dd9b707f1e8942975c1e7ed5a50ef47ec08a88ecf9d502d22f"
		"c5fac43d7caa93c45074aeaaaaa83f1e513567a4adc96ec3de9adf02e7b5883d7be59be83e9c0663f6155fb"
		"0067bf1f1705400e3a306c49bff6ab7aec0121500ad5821e4561e050f15bb975437290cd19565ff2e0");

	ASSERT_EQ(verify_signature(pk, message_digest, encrypted_digest,
		digest_algorithm::md5,
		digest_encryption_algorithm::rsa, {}), (signature_verification_result{ true, 1024u }));
}

TEST(Pkcs7SignatureTests, EcdsaSecp256k1Valid)
{
	const auto pk = hex_string_to_bytes("046318166fd2402430e43df4f385fbe5441d3190baa2a089e2cea630a"
		"e1969cbe4889fb77d3980cd98d4289ea439f10b6c9dfa8269ab5d6c28b65ca1128f473319");

	const auto message_digest = hex_string_to_bytes(
		"9834876dcfb05cb167a5c24953eba58c4ac89b1adf57f28f2f9d09af107ee8f0");

	const auto encrypted_digest = hex_string_to_bytes(
		"30450220576b2fe8c39c316b641a0878ed212d4648db0a7a19832dce628006ba382dbeff02210088be0e9b507f"
		"4761bdb908f33f94cf50bc8a2e50f47b795ea1787622e3ff3157");

	const auto params = hex_string_to_bytes("06052b8104000a");

	ASSERT_EQ(verify_signature(pk, message_digest, encrypted_digest,
		digest_algorithm::sha256,
		digest_encryption_algorithm::ecdsa, params),
		(signature_verification_result{ true, 0u, ecc_curve::secp256k1 }));
}

TEST(Pkcs7SignatureTests, EcdsaSecp256k1Invalid)
{
	const auto pk = hex_string_to_bytes("046318166fd2402430e43df4f385fbe5441d3190baa2a089e2cea630a"
		"e1969cbe4889fb77d3980cd98d4289ea439f10b6c9dfa8269ab5d6c28b65ca1128f473319");

	const auto message_digest = hex_string_to_bytes(
		"9834876dcfb05cb167a5c24953eba58c4ac89b1adf57f28f2f9d09af107ee8f0");

	const auto encrypted_digest = hex_string_to_bytes(
		"30450220576b2fe8c39c316b641a0878ed212d4648db0a7a19832dce628006ba382dbeff02210088be0e9b507f"
		"4761bdb908f33f94cf50bc8a2e50f47b795ec1787622e3ff3157");

	const auto params = hex_string_to_bytes("06052b8104000a");

	ASSERT_FALSE(verify_signature(pk, message_digest, encrypted_digest,
		digest_algorithm::sha256,
		digest_encryption_algorithm::ecdsa, params));
}

TEST(Pkcs7SignatureTests, EcdsaSecp256k1InvalidPk)
{
	const auto pk = hex_string_to_bytes("046318166fd2402430e43df4f385fbe5441d3190baa2a089e2cea630a"
		"e1969cbe4889fb77d3980cd98d4289ea439f10b6c9dfa8269ab5d6c28b65ca1128f4719");

	const auto message_digest = hex_string_to_bytes(
		"9834876dcfb05cb167a5c24953eba58c4ac89b1adf57f28f2f9d09af107ee8f0");

	const auto encrypted_digest = hex_string_to_bytes(
		"30450220576b2fe8c39c316b641a0878ed212d4648db0a7a19832dce628006ba382dbeff02210088be0e9b507f"
		"4761bdb908f33f94cf50bc8a2e50f47b795ec1787622e3ff3157");

	const auto params = hex_string_to_bytes("06052b8104000a");

	ASSERT_THROW((void)verify_signature(pk, message_digest, encrypted_digest,
		digest_algorithm::sha256,
		digest_encryption_algorithm::ecdsa, params), pe_bliss::pe_error);
}

TEST(Pkcs7SignatureTests, EcdsaSecp256k1InvalidDigest)
{
	const auto pk = hex_string_to_bytes("046318166fd2402430e43df4f385fbe5441d3190baa2a089e2cea630a"
		"e1969cbe4889fb77d3980cd98d4289ea439f10b6c9dfa8269ab5d6c28b65ca1128f473319");

	const auto message_digest = hex_string_to_bytes(
		"9834876dcfb05cb167a5c24953eba58c4ac89b1adf57f28f2f9d09af1078f0");

	const auto encrypted_digest = hex_string_to_bytes(
		"30450220576b2fe8c39c316b641a0878ed212d4648db0a7a19832dce628006ba382dbeff02210088be0e9b507f"
		"4761bdb908f33f94cf50bc8a2e50f47b795ea1787622e3ff3157");

	const auto params = hex_string_to_bytes("06052b8104000a");

	ASSERT_THROW((void)verify_signature(pk, message_digest, encrypted_digest,
		digest_algorithm::sha256,
		digest_encryption_algorithm::ecdsa, params), pe_bliss::pe_error);
}

TEST(Pkcs7SignatureTests, EcdsaSecp521r1Sha1Valid)
{
	const auto pk = hex_string_to_bytes("04000d7eed672cd7b62feab41a1d7bbf11447cec5d4e23c46d2c1246e2"
		"45e3fe24ad11591ca3b98e41f17cccfb8b8ecf345e1fd8f2af4112ab4157864e2d3b113ac7e2006de494c5aa01"
		"0d7b13b87af839ef2e226f9cab88c25622b56c01495f50c1d0ddbb9be38b3814c22ed9eb394daa1083dac0f695"
		"01831ff9ab4e1b3d54c6a05546d9");

	const auto message_digest = hex_string_to_bytes(
		"7e240de74fb1ed08fa08d38063f6a6a91462a815");

	const auto encrypted_digest = hex_string_to_bytes(
		"30818802420180040deaa9f8c79647b2ff1b485a6ff400131d0ac56cf70ba8823cf3f3173d26487e45b82d48cd7"
		"e2ee220eef4856036ec3b26a8c307ed1f31697b72731775a10f02420097ed10e12031f9a1fd4371ecb289a5436e"
		"d2e87f257997760b427999eb6f69bdb24f26acb875cc3b8c2dc4aa48dcf2b25da8e619ee30324add1b29026b30f"
		"45510");

	const auto params = hex_string_to_bytes("06052b81040023");

	ASSERT_EQ(verify_signature(pk, message_digest, encrypted_digest,
		digest_algorithm::sha1,
		digest_encryption_algorithm::ecdsa, params),
		(signature_verification_result{ true, 0u, ecc_curve::secp521r1 }));
}
