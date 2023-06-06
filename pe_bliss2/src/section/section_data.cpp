#include "pe_bliss2/section/section_data.h"

#include <cstddef>
#include <exception>
#include <memory>
#include <system_error>

#include "buffers/input_buffer_section.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_virtual_buffer.h"
#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/section/section_errc.h"
#include "pe_bliss2/section/section_header.h"
#include "pe_bliss2/section/section_table.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/generic_error.h"
#include "utilities/safe_uint.h"

namespace pe_bliss::section
{

void section_data::deserialize(const section_header& header,
	buffers::input_buffer_stateful_wrapper& buffer,
	const section_data_load_options& options)
{
	auto raw_size = header.get_raw_size(options.section_alignment);
	auto virtual_size = header.get_virtual_size(options.section_alignment);

	try
	{
		utilities::safe_uint<std::size_t> buffer_pos(options.image_loaded_to_memory
			? header.get_descriptor()->virtual_address
			: header.get_pointer_to_raw_data());
		buffer_pos += options.image_start_buffer_pos;

		std::shared_ptr<buffers::input_buffer_interface> buffer_section
			= std::make_shared<buffers::input_buffer_section>(
				buffer.get_buffer(), buffer_pos.value(), raw_size);
		buffer_section->set_relative_offset(0);

		if (virtual_size > raw_size)
		{
			buffer_section = std::make_shared<buffers::input_virtual_buffer>(
				std::move(buffer_section), virtual_size - raw_size);
		}

		ref_buffer::deserialize(buffer_section, options.copy_memory);
	}
	catch (const std::system_error&)
	{
		std::throw_with_nested(pe_error(
			section_errc::unable_to_read_section_data));
	}
}

} //namespace pe_bliss::section
