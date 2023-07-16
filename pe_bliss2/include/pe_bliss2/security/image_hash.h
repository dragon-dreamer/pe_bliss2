#pragma once

#include <cstddef>
#include <optional>
#include <system_error>
#include <type_traits>
#include <vector>

#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/crypto_algorithms.h"

namespace pe_bliss::image { class image; }

namespace pe_bliss::security
{

enum class hash_calculator_errc
{
	invalid_security_directory_offset,
	invalid_section_data,
	too_big_page_hash_buffer
};

std::error_code make_error_code(hash_calculator_errc) noexcept;

struct [[nodiscard]] image_hash_result
{
	std::vector<std::byte> image_hash;
	std::vector<std::byte> page_hashes;
	std::error_code page_hash_errc{};
};

struct [[nodiscard]] page_hash_options final
{
	digest_algorithm algorithm{ digest_algorithm::unknown };
	std::size_t max_page_hashes_size { 10u * 1024u * 1024u }; //10 Mb
};

[[nodiscard]]
image_hash_result calculate_hash(digest_algorithm algorithm,
	const pe_bliss::image::image& instance,
	const page_hash_options* page_hash_opts = nullptr);

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::hash_calculator_errc> : true_type {};
} //namespace std
