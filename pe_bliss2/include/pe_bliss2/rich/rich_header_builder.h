#pragma once

#include <cstddef>
#include <system_error>
#include <type_traits>

namespace buffers
{
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::rich
{

enum class rich_header_builder_errc
{
	unable_to_build_rich_header = 1
};

class rich_header;

//buffer should contain DOS stub. Rich header will be
//written to this buffer. If necessary, the buffer will
//be extended.
void build(const rich_header& header,
	buffers::output_buffer_interface& buffer);

[[nodiscard]]
std::size_t get_built_size(const rich_header& header);

std::error_code make_error_code(rich_header_builder_errc e) noexcept;

} // namespace pe_bliss::rich

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::rich::rich_header_builder_errc> : true_type {};
} //namespace std
