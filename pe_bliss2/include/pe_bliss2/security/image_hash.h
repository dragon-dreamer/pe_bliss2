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
	invalid_security_directory_offset = 1,
	invalid_section_data,
	page_hashes_data_too_big
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

struct [[nodiscard]] image_hash_verification_result
{
	bool image_hash_valid{};
	std::optional<bool> page_hashes_valid;
	std::error_code page_hashes_check_errc;

	[[nodiscard]]
	explicit operator bool() const noexcept
	{
		return image_hash_valid
			&& (!page_hashes_valid || *page_hashes_valid)
			&& !page_hashes_check_errc;
	}
};

[[nodiscard]]
image_hash_verification_result verify_image_hash(span_range_type image_hash,
	digest_algorithm digest_alg, const image::image& instance,
	const std::optional<span_range_type>& page_hashes,
	const std::optional<page_hash_options>& page_hash_options);

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::hash_calculator_errc> : true_type {};
} //namespace std
