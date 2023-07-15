#pragma once

#include <cstddef>
#include <system_error>
#include <span>
#include <type_traits>
#include <vector>

#include "buffers/input_buffer_interface.h"

#include "pe_bliss2/security/crypto_algorithms.h"

namespace pe_bliss::security
{

enum class buffer_hash_errc
{
	unsupported_hash_algorithm
};

std::error_code make_error_code(buffer_hash_errc) noexcept;

[[nodiscard]]
std::vector<std::byte> calculate_hash(digest_algorithm algorithm,
	buffers::input_buffer_interface& buffer);

[[nodiscard]]
std::vector<std::byte> calculate_hash(digest_algorithm algorithm,
	std::span<const std::span<const std::byte>> buffers);

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::buffer_hash_errc> : true_type {};
} //namespace std
