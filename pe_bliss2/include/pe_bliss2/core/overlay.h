#pragma once

#include <cstddef>
#include <cstdint>
#include <system_error>
#include <type_traits>

#include "buffers/ref_buffer.h"

namespace buffers
{
class input_buffer_stateful_wrapper;
} //namespace buffers

namespace pe_bliss::core
{

enum class overlay_errc
{
	unable_to_read_overlay = 1
};

class [[nodiscard]] overlay : private buffers::ref_buffer
{
public:
	using ref_buffer::serialize;
	using ref_buffer::data;
	using ref_buffer::copied_data;
	using ref_buffer::copy_referenced_buffer;
	using ref_buffer::size;
	using ref_buffer::virtual_size;
	using ref_buffer::physical_size;
	using ref_buffer::is_stateless;
	using ref_buffer::is_copied;

	void deserialize(std::uint64_t section_raw_data_last_offset,
		std::uint32_t size_of_headers,
		std::size_t initial_buffer_pos,
		buffers::input_buffer_stateful_wrapper& buffer,
		bool eager_copy);

	[[nodiscard]]
	buffers::ref_buffer& get_buffer() noexcept
	{
		return static_cast<buffers::ref_buffer&>(*this);
	}

	[[nodiscard]]
	const buffers::ref_buffer& get_buffer() const noexcept
	{
		return static_cast<const buffers::ref_buffer&>(*this);
	}
};

std::error_code make_error_code(overlay_errc) noexcept;

} //namespace pe_bliss::core

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::core::overlay_errc> : true_type {};
} //namespace std
