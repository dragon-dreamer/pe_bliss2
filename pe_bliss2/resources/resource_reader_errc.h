#pragma once

#include <system_error>
#include <type_traits>

namespace pe_bliss::resources
{

enum class resource_reader_errc
{
	invalid_buffer_size = 1,
	buffer_read_error
};

std::error_code make_error_code(resource_reader_errc) noexcept;

} //namespace pe_bliss::resources

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::resources::resource_reader_errc> : true_type {};
} //namespace std
