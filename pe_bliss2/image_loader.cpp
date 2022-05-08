#include "pe_bliss2/image_loader.h"

#include <cassert>
#include <cstdint>
#include <string>

#include "buffers/input_buffer_section.h"
#include "pe_bliss2/dos/dos_header.h"
#include "pe_bliss2/dos/dos_header_errc.h"
#include "pe_bliss2/dos/dos_header_validator.h"
#include "pe_bliss2/core/image_signature_validator.h"
#include "pe_bliss2/core/optional_header_validator.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/section/pe_section_error.h"
#include "pe_bliss2/section/section_data.h"
#include "utilities/math.h"

namespace pe_bliss
{

image image_loader::load(const buffers::input_buffer_ptr& buffer,
	const image_load_options& options)
{
	assert(!options.load_section_data || options.validate_sections);
	assert(buffer);

	image instance;
	instance.set_loaded_to_memory(options.image_loaded_to_memory);

	auto& dos_hdr = instance.get_dos_header();
	dos_hdr.deserialize(*buffer, options.allow_virtual_headers);
	dos::validate(dos_hdr, options.dos_header_validation).throw_on_error();

	instance.get_dos_stub().deserialize(buffer, {
		.copy_memory = options.eager_dos_stub_data_copy,
		.e_lfanew = dos_hdr.base_struct()->e_lfanew
	});

	std::size_t pe_headers_start = dos_hdr.base_struct().get_state().buffer_pos();
	if (!utilities::math::add_if_safe<std::size_t>(pe_headers_start, dos_hdr.base_struct()->e_lfanew))
		throw pe_error(dos::dos_header_errc::invalid_e_lfanew);

	buffer->set_rpos(pe_headers_start);

	instance.get_image_signature().deserialize(*buffer, options.allow_virtual_headers);
	validate(instance.get_image_signature()).throw_on_error();

	auto& file_hdr = instance.get_file_header();
	auto& optional_hdr = instance.get_optional_header();
	file_hdr.deserialize(*buffer, options.allow_virtual_headers);
	optional_hdr.deserialize(*buffer, options.allow_virtual_headers);
	if (options.validate_size_of_optional_header)
	{
		validate_size_of_optional_header(file_hdr.base_struct()->size_of_optional_header,
			optional_hdr).throw_on_error();
	}
	validate(optional_hdr, options.optional_header_validation,
		file_hdr.is_dll()).throw_on_error();

	instance.get_data_directories().deserialize(*buffer,
		optional_hdr.get_number_of_rva_and_sizes(), options.allow_virtual_headers);

	if (options.validate_image_base)
		validate_image_base(optional_hdr, instance.has_relocation()).throw_on_error();

	buffer->set_rpos(file_hdr.get_section_table_buffer_pos());
	auto& section_tbl = instance.get_section_table();
	section_tbl.deserialize(*buffer, file_hdr.base_struct()->number_of_sections,
		options.allow_virtual_headers);

	if (options.validate_sections)
	{
		auto& sections = instance.get_section_data_list();
		section::section_data_load_options load_opts{
			.section_alignment = optional_hdr.get_raw_section_alignment(),
			.copy_memory = options.eager_section_data_copy,
			.image_loaded_to_memory = options.image_loaded_to_memory,
			.image_start_buffer_pos = dos_hdr.base_struct().get_state().buffer_pos()
		};

		pe_error_wrapper result;
		std::size_t section_index = 0;
		for (auto it = section_tbl.get_section_headers().cbegin(),
			end = section_tbl.get_section_headers().cend(); it != end; ++it, ++section_index)
		{
			if ((result = section_tbl.validate_section_header(optional_hdr, it)))
				throw section::pe_section_error(result, section_index, std::string(it->get_name()));
		}

		if (options.load_section_data)
		{
			for (auto it = section_tbl.get_section_headers().cbegin(),
				end = section_tbl.get_section_headers().cend(); it != end; ++it)
			{
				//TODO: may throw - catch?
				sections.emplace_back().deserialize(*it, buffer, load_opts);
			}
		}

		if (options.validate_size_of_image)
			section_tbl.validate_size_of_image(optional_hdr).throw_on_error();

		if (options.load_overlay && !options.image_loaded_to_memory)
		{
			std::uint32_t last_section_offset = 0;
			for (auto it = section_tbl.get_section_headers().cbegin(),
				end = section_tbl.get_section_headers().cend(); it != end; ++it)
			{
				last_section_offset = (std::max)(last_section_offset,
					it->get_pointer_to_raw_data() + it->get_raw_size(
						optional_hdr.get_raw_section_alignment()));
			}

			if (buffer->size() > last_section_offset)
			{
				buffer->set_rpos(last_section_offset);
				std::size_t overlay_size = buffer->size() - last_section_offset;
				instance.get_overlay().deserialize(
					std::make_shared<buffers::input_buffer_section>(buffer, last_section_offset, overlay_size),
					options.eager_overlay_data_copy);
			}
		}
	}

	//TODO: make dos, file, optional and other headers reference this data instead of copying its parts
	if (options.load_full_headers_buffer)
	{
		auto start_pos = dos_hdr.base_struct().get_state().buffer_pos();
		auto size = optional_hdr.get_raw_size_of_headers(); //TODO may be invalid
		buffer->set_rpos(start_pos);
		instance.get_full_headers_buffer().deserialize(
			std::make_shared<buffers::input_buffer_section>(buffer, start_pos, size),
			options.eager_full_headers_buffer_copy);
	}

	return instance;
}

} //namespace pe_bliss
