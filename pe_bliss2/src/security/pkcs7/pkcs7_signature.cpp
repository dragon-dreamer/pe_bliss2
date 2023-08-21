#include "pe_bliss2/security/pkcs7/pkcs7_signature.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <exception>
#include <string>
#include <system_error>

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "cryptopp/asn.h"
#include "cryptopp/cryptlib.h"
#include "cryptopp/dsa.h"
#include "cryptopp/eccrypto.h"
#include "cryptopp/filters.h"
#include "cryptopp/md5.h"
#include "cryptopp/pkcspad.h"
#include "cryptopp/rsa.h"
#include "cryptopp/sha.h"

#include "pe_bliss2/pe_error.h"

namespace
{
template<typename Hash>
class IdentityHash : public CryptoPP::HashTransformation
{
public:
	static constexpr std::size_t DIGESTSIZE = Hash::DIGESTSIZE;

	void Update(const CryptoPP::byte* src, [[maybe_unused]] size_t size) final
	{
		assert(size == DIGESTSIZE);
		std::memcpy(digest.data(), src, DIGESTSIZE);
	}

	void TruncatedFinal(CryptoPP::byte* result, [[maybe_unused]] size_t size) final
	{
		assert(size == DIGESTSIZE);
		std::memcpy(result, digest.data(), DIGESTSIZE);
	}

	unsigned int DigestSize() const final { return DIGESTSIZE; }

	static constexpr const char* StaticAlgorithmName() { return "TRANSPARENT"; }

	std::string AlgorithmProvider() const final { return "TRANSPARENT"; }

private:
	std::array<char, DIGESTSIZE> digest;
};

template<typename Hash>
class IdentityHashNoIdentity final : public IdentityHash<Hash> {};

using IdentitySHA512 = IdentityHash<CryptoPP::SHA512>;
using IdentitySHA384 = IdentityHash<CryptoPP::SHA384>;
using IdentitySHA256 = IdentityHash<CryptoPP::SHA256>;
using IdentitySHA1 = IdentityHash<CryptoPP::SHA1>;
using IdentityMD5 = IdentityHash<CryptoPP::Weak::MD5>;

using IdentitySHA512NoIdentity = IdentityHashNoIdentity<CryptoPP::SHA512>;
using IdentitySHA384NoIdentity = IdentityHashNoIdentity<CryptoPP::SHA384>;
using IdentitySHA256NoIdentity = IdentityHashNoIdentity<CryptoPP::SHA256>;
using IdentitySHA1NoIdentity = IdentityHashNoIdentity<CryptoPP::SHA1>;
using IdentityMD5NoIdentity = IdentityHashNoIdentity<CryptoPP::Weak::MD5>;

template<typename Verifier>
bool verify_signature_impl(Verifier& verifier,
	std::span<const std::byte> message_digest,
	std::span<const std::byte> encrypted_digest)
{
	return verifier.VerifyMessage(
		reinterpret_cast<const CryptoPP::byte*>(message_digest.data()),
		message_digest.size(),
		reinterpret_cast<const CryptoPP::byte*>(encrypted_digest.data()),
		encrypted_digest.size());
}

struct signature_validator_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "signature_validator";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::pkcs7::signature_validator_errc;
		switch (static_cast<pe_bliss::security::pkcs7::signature_validator_errc>(ev))
		{
		case invalid_signature:
			return "Invalid digital signature";
		case unsupported_signature_algorithm:
			return "Unsupported digital signature or hash algorithm";
		default:
			return {};
		}
	}
};

const signature_validator_error_category signature_validator_error_category_instance;

template<typename Hash>
bool verify_rsa_pkcs1v15_signature(const CryptoPP::RSA::PublicKey& public_key,
	std::span<const std::byte> message_digest,
	std::span<const std::byte> encrypted_digest)
{
	typename CryptoPP::RSASS<CryptoPP::PKCS1v15, IdentityHash<Hash>>
		::Verifier verifier(public_key);
	if (verify_signature_impl(verifier, message_digest, encrypted_digest))
		return true;

	typename CryptoPP::RSASS<CryptoPP::PKCS1v15, IdentityHashNoIdentity<Hash>>
		::Verifier no_identity_verifier(public_key);
	return verify_signature_impl(
		no_identity_verifier, message_digest, encrypted_digest);
}

} //namespace

template<> const CryptoPP::byte CryptoPP::PKCS_DigestDecoration<IdentitySHA512>::decoration[]{
	0x30,0x51,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x03,0x05,0x00,0x04,0x40 };
template<> const unsigned int CryptoPP::PKCS_DigestDecoration<IdentitySHA512>::length
	= sizeof(PKCS_DigestDecoration<IdentitySHA512>::decoration);

template<> const CryptoPP::byte CryptoPP::PKCS_DigestDecoration<IdentitySHA384>::decoration[]{
	0x30,0x41,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x02,0x05,0x00,0x04,0x30 };
template<> const unsigned int CryptoPP::PKCS_DigestDecoration<IdentitySHA384>::length
	= sizeof(PKCS_DigestDecoration<IdentitySHA384>::decoration);

template<> const CryptoPP::byte CryptoPP::PKCS_DigestDecoration<IdentitySHA256>::decoration[]{
	0x30,0x31,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x01,0x05,0x00,0x04,0x20 };
template<> const unsigned int CryptoPP::PKCS_DigestDecoration<IdentitySHA256>::length
	= sizeof(PKCS_DigestDecoration<IdentitySHA256>::decoration);

template<> const CryptoPP::byte CryptoPP::PKCS_DigestDecoration<IdentitySHA1>::decoration[]{
	0x30,0x21,0x30,0x09,0x06,0x05,0x2B,0x0E,0x03,0x02,0x1A,0x05,0x00,0x04,0x14 };
template<> const unsigned int CryptoPP::PKCS_DigestDecoration<IdentitySHA1>::length
	= sizeof(PKCS_DigestDecoration<IdentitySHA1>::decoration);

template<> const CryptoPP::byte CryptoPP::PKCS_DigestDecoration<IdentityMD5>::decoration[]{
	0x30,0x20,0x30,0x0c,0x06,0x08,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,0x05,0x05,0x00,0x04,0x10 };
template<> const unsigned int CryptoPP::PKCS_DigestDecoration<IdentityMD5>::length
	= sizeof(PKCS_DigestDecoration<IdentityMD5>::decoration);

template<> const CryptoPP::byte CryptoPP::PKCS_DigestDecoration<IdentitySHA512NoIdentity>::decoration[]{0};
template<> const unsigned int CryptoPP::PKCS_DigestDecoration<IdentitySHA512NoIdentity>::length = 0;
template<> const CryptoPP::byte CryptoPP::PKCS_DigestDecoration<IdentitySHA384NoIdentity>::decoration[]{ 0 };
template<> const unsigned int CryptoPP::PKCS_DigestDecoration<IdentitySHA384NoIdentity>::length = 0;
template<> const CryptoPP::byte CryptoPP::PKCS_DigestDecoration<IdentitySHA256NoIdentity>::decoration[]{ 0 };
template<> const unsigned int CryptoPP::PKCS_DigestDecoration<IdentitySHA256NoIdentity>::length = 0;
template<> const CryptoPP::byte CryptoPP::PKCS_DigestDecoration<IdentitySHA1NoIdentity>::decoration[]{ 0 };
template<> const unsigned int CryptoPP::PKCS_DigestDecoration<IdentitySHA1NoIdentity>::length = 0;
template<> const CryptoPP::byte CryptoPP::PKCS_DigestDecoration<IdentityMD5NoIdentity>::decoration[]{ 0 };
template<> const unsigned int CryptoPP::PKCS_DigestDecoration<IdentityMD5NoIdentity>::length = 0;

namespace pe_bliss::security::pkcs7
{

namespace
{

bool verify_rsa_signature(CryptoPP::ArraySource& key_bytes,
	std::span<const std::byte> message_digest,
	std::span<const std::byte> encrypted_digest,
	digest_algorithm digest_alg)
{
	CryptoPP::RSA::PublicKey public_key;
	public_key.BERDecodePublicKey(key_bytes, false, 0u);
	switch (digest_alg)
	{
	case digest_algorithm::sha512:
		return verify_rsa_pkcs1v15_signature<CryptoPP::SHA512>(public_key,
			message_digest, encrypted_digest);
	case digest_algorithm::sha384:
		return verify_rsa_pkcs1v15_signature<CryptoPP::SHA384>(public_key,
			message_digest, encrypted_digest);
	case digest_algorithm::sha256:
		return verify_rsa_pkcs1v15_signature<CryptoPP::SHA256>(public_key,
			message_digest, encrypted_digest);
	case digest_algorithm::sha1:
		return verify_rsa_pkcs1v15_signature<CryptoPP::SHA1>(public_key,
			message_digest, encrypted_digest);
	case digest_algorithm::md5:
		return verify_rsa_pkcs1v15_signature<CryptoPP::Weak::MD5>(public_key,
			message_digest, encrypted_digest);
	default:
		break;
	}

	throw pe_error(signature_validator_errc::unsupported_signature_algorithm);
}

bool verify_ecdsa_signature(CryptoPP::ArraySource& key_bytes,
	std::span<const std::byte> message_digest,
	std::span<const std::byte> encrypted_digest,
	const std::span<const std::byte>* signature_algorithm_parameters)
{
	CryptoPP::OID oid;
	CryptoPP::ArraySource signature_algorithm_parameters_bytes(
		reinterpret_cast<const CryptoPP::byte*>(signature_algorithm_parameters->data()),
		signature_algorithm_parameters->size(), true);
	oid.BERDecode(signature_algorithm_parameters_bytes);

	CryptoPP::DL_Keys_ECDSA<CryptoPP::ECP>::PublicKey pk;
	CryptoPP::ECP::Point q;
	CryptoPP::DL_GroupParameters_EC<CryptoPP::ECP> params(oid);
	if (!params.GetCurve().DecodePoint(q, key_bytes, key_bytes.TotalBytesRetrievable()))
		throw pe_error(signature_validator_errc::invalid_signature);

	pk.Initialize(params, q);

	CryptoPP::ECDSA<CryptoPP::ECP, IdentitySHA256>::Verifier verifier(pk);

	CryptoPP::SecByteBlock signature(verifier.SignatureLength());
	CryptoPP::DSAConvertSignatureFormat(signature, signature.size(), CryptoPP::DSA_P1363,
		reinterpret_cast<const CryptoPP::byte*>(encrypted_digest.data()),
		encrypted_digest.size(), CryptoPP::DSA_DER);

	return verifier.VerifyMessage(
		reinterpret_cast<const CryptoPP::byte*>(message_digest.data()),
		message_digest.size(),
		signature,
		signature.size());
}

} //namespace

std::error_code make_error_code(signature_validator_errc e) noexcept
{
	return { static_cast<int>(e), signature_validator_error_category_instance };
}

bool verify_signature(std::span<const std::byte> raw_public_key,
	std::span<const std::byte> message_digest,
	std::span<const std::byte> encrypted_digest,
	digest_algorithm digest_alg,
	digest_encryption_algorithm encryption_alg,
	const std::span<const std::byte>* signature_algorithm_parameters)
{
	try
	{
		CryptoPP::ArraySource key_bytes(
			reinterpret_cast<const CryptoPP::byte*>(raw_public_key.data()),
			raw_public_key.size(), true);

		if (encryption_alg == digest_encryption_algorithm::rsa)
		{
			return verify_rsa_signature(key_bytes,
				message_digest, encrypted_digest, digest_alg);
		}

		if (encryption_alg == digest_encryption_algorithm::ecdsa)
		{
			return verify_ecdsa_signature(key_bytes,
				message_digest, encrypted_digest, signature_algorithm_parameters);
		}
	}
	catch (const CryptoPP::Exception&)
	{
		std::throw_with_nested(pe_error(signature_validator_errc::invalid_signature));
	}

	throw pe_error(signature_validator_errc::unsupported_signature_algorithm);
}

} //namespace pe_bliss::security::pkcs7
