#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>

namespace pe_bliss::image
{

enum class checksum_errc
{
	invalid_checksum_offset = 1,
	unaligned_checksum,
	unaligned_buffer
};

std::error_code make_error_code(checksum_errc) noexcept;

class image;

using image_checksum_type = std::uint32_t;

[[nodiscard]]
image_checksum_type calculate_checksum(const image& instance);

} //namespace pe_bliss::image

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::image::checksum_errc> : true_type {};
} //namespace std
