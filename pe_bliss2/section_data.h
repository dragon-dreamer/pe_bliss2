#pragma once

#include <cstddef>
#include <cstdint>

#include "buffers/input_buffer_interface.h"
#include "buffers/ref_buffer.h"

namespace buffers
{
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss
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
	using ref_buffer::empty;
	using ref_buffer::size;

	void deserialize(const section_header& header,
		const buffers::input_buffer_ptr& buffer,
		const section_data_load_options& options);
};

} //namespace pe_bliss
