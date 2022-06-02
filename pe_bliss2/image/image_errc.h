#pragma once

#include <system_error>

namespace pe_bliss::image
{

enum class image_errc
{
	too_many_sections = 1,
	too_many_rva_and_sizes,
	section_data_does_not_exist
};

std::error_code make_error_code(image_errc) noexcept;

} //namespace pe_bliss::image

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::image::image_errc> : true_type {};
} //namespace std
