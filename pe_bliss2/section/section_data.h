#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "buffers/ref_buffer.h"

namespace buffers
{
class input_buffer_stateful_wrapper;
} //namespace buffers

namespace pe_bliss::section
{

class section_header;

struct [[nodiscard]] section_data_load_options
{
	std::uint32_t section_alignment = 0;
	bool copy_memory = true;
	bool image_loaded_to_memory = false;
	std::size_t image_start_buffer_pos = 0;
};

// When deserializing, buffer pos may be anything. Buffer is repositioned
// using section_data_load_options values.
class [[nodiscard]] section_data : private buffers::ref_buffer
{
public:
	using ref_buffer::serialize;
	using ref_buffer::data;
	using ref_buffer::copied_data;
	using ref_buffer::copy_referenced_buffer;
	using ref_buffer::virtual_size;
	using ref_buffer::physical_size;
	using ref_buffer::is_stateless;
	using ref_buffer::size;
	using ref_buffer::is_copied;

	void deserialize(const section_header& header,
		buffers::input_buffer_stateful_wrapper& buffer,
		const section_data_load_options& options);

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

using section_data_list = std::vector<section_data>;

} //namespace pe_bliss::section
