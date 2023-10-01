#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/security/crypto_algorithms.h"

namespace pe_bliss::security::pkcs7
{

enum class signature_validator_errc
{
	invalid_signature,
	unsupported_signature_algorithm
};

std::error_code make_error_code(signature_validator_errc) noexcept;

enum class ecc_curve
{
	unknown = 0,
	sm2p256v1,
	sm2encrypt_recommendedParameters,
	secp192r1,
	secp256r1,
	brainpoolP160r1,
	brainpoolP192r1,
	brainpoolP224r1,
	brainpoolP256r1,
	brainpoolP320r1,
	brainpoolP384r1,
	brainpoolP512r1,
	secp112r1,
	secp112r2,
	secp160r1,
	secp160k1,
	secp256k1,
	secp128r1,
	secp128r2,
	secp160r2,
	secp192k1,
	secp224k1,
	secp224r1,
	secp384r1,
	secp521r1
};

struct signature_verification_result
{
	bool valid{};
	std::size_t key_size{}; //For RSA signatures
	ecc_curve curve{}; //For ECDSA signatures

	[[nodiscard]]
	operator bool() const noexcept
	{
		return valid;
	}

	[[nodiscard]]
	bool operator==(const signature_verification_result&) const noexcept = default;
};

[[nodiscard]]
signature_verification_result verify_signature(std::span<const std::byte> raw_public_key,
	std::span<const std::byte> message_digest,
	std::span<const std::byte> encrypted_digest,
	digest_algorithm digest_alg,
	digest_encryption_algorithm encryption_alg,
	std::span<const std::byte> signature_algorithm_parameters);

} //namespace pe_bliss::security::pkcs7

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::pkcs7::signature_validator_errc> : true_type {};
} //namespace std
