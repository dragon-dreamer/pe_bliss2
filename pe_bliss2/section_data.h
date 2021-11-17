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

struct section_data_load_options
{
	std::uint32_t section_alignment = 0;
	bool copy_memory = true;
	bool image_loaded_to_memory = false;
	std::size_t image_start_buffer_pos = 0;
};

class section_data
{
public:
	void deserialize(const section_header& header,
		const buffers::input_buffer_ptr& buffer,
		const section_data_load_options& options);
	void serialize(buffers::output_buffer_interface& buffer) const;
	
	[[nodiscard]]
	buffers::input_buffer_ptr data() const;

	[[nodiscard]]
	buffers::ref_buffer::container_type& copied_data();

	[[nodiscard]]
	const buffers::ref_buffer::container_type& copied_data() const;

	void copy_referenced_buffer();

	[[nodiscard]] bool empty() const noexcept;
	[[nodiscard]] std::size_t size() const noexcept;

private:
	buffers::ref_buffer buffer_;
};

} //namespace pe_bliss
