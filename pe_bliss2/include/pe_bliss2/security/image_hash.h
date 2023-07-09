#pragma once

#include <cstddef>
#include <system_error>
#include <type_traits>
#include <vector>

#include "pe_bliss2/security/authenticode_pkcs7.h"

namespace pe_bliss::image { class image; }

namespace pe_bliss::security
{

enum class hash_calculator_errc
{
	invalid_security_directory_offset,
	invalid_section_data,
	unable_to_read_image_data,
	unsupported_hash_algorithm,
	no_signers
};

std::error_code make_error_code(hash_calculator_errc) noexcept;

[[nodiscard]]
std::vector<std::byte> calculate_hash(pkcs7::digest_algorithm algorithm,
	const pe_bliss::image::image& instance);

template<typename RangeType>
[[nodiscard]]
bool is_hash_valid(const authenticode_pkcs7<RangeType>& signature,
	const pe_bliss::image::image& instance);

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::hash_calculator_errc> : true_type {};
} //namespace std
