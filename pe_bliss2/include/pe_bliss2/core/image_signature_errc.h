#pragma once

#include <system_error>
#include <type_traits>

namespace pe_bliss::core
{

enum class image_signature_errc
{
	invalid_pe_signature = 1,
	unable_to_read_pe_signature
};

std::error_code make_error_code(image_signature_errc) noexcept;

} //namespace pe_bliss::core

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::core::image_signature_errc> : true_type {};
} //namespace std
