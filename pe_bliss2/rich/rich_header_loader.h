#pragma once

#include <cstddef>
#include <optional>
#include <system_error>
#include <type_traits>

namespace buffers
{
class input_buffer_interface;
} //namespace buffers

namespace pe_bliss::rich
{

class rich_header;

enum class rich_header_loader_errc
{
	no_dans_signature = 1,
	unable_to_decode_compids
};

std::error_code make_error_code(rich_header_loader_errc) noexcept;

// buffer should contain DOS stub data
std::optional<rich_header> load(buffers::input_buffer_interface& buffer);

} // namespace pe_bliss::rich

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::rich::rich_header_loader_errc> : true_type {};
} //namespace std
