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

[[nodiscard]]
bool verify_signature(std::span<const std::byte> raw_public_key,
	std::span<const std::byte> message_digest,
	std::span<const std::byte> encrypted_digest,
	digest_algorithm digest_alg,
	digest_encryption_algorithm encryption_alg,
	const std::span<const std::byte>* signature_algorithm_parameters);

} //namespace pe_bliss::security::pkcs7

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::pkcs7::signature_validator_errc> : true_type {};
} //namespace std
