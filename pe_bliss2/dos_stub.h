#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>

#include "buffers/input_buffer_interface.h"
#include "buffers/ref_buffer.h"

namespace pe_bliss
{

enum class dos_stub_errc
{
	unable_to_read_dos_stub = 1
};

struct [[nodiscard]] dos_stub_load_options
{
	bool copy_memory = true;
	std::uint32_t e_lfanew = 0;
};

std::error_code make_error_code(dos_stub_errc) noexcept;

//When deserializing, buf should point to the beginning of the DOS stub
class [[nodiscard]] dos_stub : private buffers::ref_buffer
{
public:
	using ref_buffer::serialize;
	using ref_buffer::data;
	using ref_buffer::copied_data;
	using ref_buffer::copy_referenced_buffer;
	using ref_buffer::empty;
	using ref_buffer::size;

	//When deserializing, buf should point to DOS stub start (right after DOS header)
	void deserialize(const buffers::input_buffer_ptr& buffer,
		const dos_stub_load_options& options);
};

} //namespace pe_bliss

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::dos_stub_errc> : true_type {};
} //namespace std
