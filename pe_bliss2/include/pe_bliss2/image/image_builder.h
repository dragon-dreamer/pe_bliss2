#pragma once

#include <system_error>
#include <type_traits>

#include "utilities/static_class.h"

namespace buffers
{
class output_buffer_interface;
} //namespace detail

namespace pe_bliss::image
{

class image;

enum class image_builder_errc
{
	inconsistent_section_headers_and_data = 1,
	invalid_section_table_offset
};

std::error_code make_error_code(image_builder_errc) noexcept;

struct [[nodiscard]] image_builder_options
{
	bool write_structure_virtual_parts = false;
	bool fill_full_headers_data_gaps = true;
};

class image_builder : public utilities::static_class
{
public:
	static void build(const image& instance, buffers::output_buffer_interface& buffer,
		const image_builder_options& options = {});
};

} //namespace pe_bliss::image

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::image::image_builder_errc> : true_type {};
} //namespace std
