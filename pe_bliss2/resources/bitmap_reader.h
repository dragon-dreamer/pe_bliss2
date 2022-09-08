#pragma once

#include <system_error>
#include <type_traits>

#include "buffers/input_buffer_interface.h"

namespace pe_bliss::resources
{

class bitmap;

enum class bitmap_reader_errc
{
	invalid_bitmap_header = 1,
	invalid_buffer_size,
	buffer_read_error
};

std::error_code make_error_code(bitmap_reader_errc) noexcept;

[[nodiscard]]
bitmap bitmap_from_resource(buffers::input_buffer_ptr buf,
	bool allow_virtual_memory);

} //namespace pe_bliss::resources

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::resources::bitmap_reader_errc> : true_type {};
} //namespace std
