#pragma once

#include <optional>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/image.h"
#include "utilities/static_class.h"

namespace buffers
{
class output_buffer_interface;
} //namespace detail

namespace pe_bliss
{

enum class image_builder_errc
{
	inconsistent_section_headers_and_data = 1
};

std::error_code make_error_code(image_builder_errc) noexcept;

struct image_builder_options
{
	bool write_structure_virtual_parts = true;
	bool fill_full_headers_data_gaps = true;
	std::optional<std::size_t> rich_header_dos_stub_offset;
};

class image_builder : public utilities::static_class
{
public:
	static void build(const image& instance, const image_builder_options& options,
		buffers::output_buffer_interface& buffer);
};

} //namespace pe_bliss

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::image_builder_errc> : true_type {};
} //namespace std
