#include "pe_bliss2/image/image_loader.h"

#include <cassert>
#include <cstdint>
#include <exception>
#include <iterator>
#include <string>

#include "buffers/input_buffer_section.h"
#include "pe_bliss2/dos/dos_header.h"
#include "pe_bliss2/dos/dos_header_errc.h"
#include "pe_bliss2/dos/dos_header_validator.h"
#include "pe_bliss2/core/image_signature_validator.h"
#include "pe_bliss2/core/optional_header_validator.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/section/section_data.h"
#include "pe_bliss2/section/section_errc.h"
#include "pe_bliss2/section/section_table_validator.h"
#include "utilities/math.h"

namespace pe_bliss::image
{

image_load_result image_loader::load(const buffers::input_buffer_ptr& buffer,
	const image_load_options& options)
{
	assert(!options.load_section_data || options.validate_sections);
	assert(buffer);

	image_load_result result;
	image& instance = result.image;

	try
	{
		instance.set_loaded_to_memory(options.image_loaded_to_memory);

		auto& dos_hdr = instance.get_dos_header();
		dos_hdr.deserialize(*buffer, options.allow_virtual_headers);
		if (options.dos_header_validation.validate_magic)
			dos::validate_magic(dos_hdr).throw_on_error();
		if (options.dos_header_validation.validate_e_lfanew)
			dos::validate_e_lfanew(dos_hdr).throw_on_error();

		instance.get_dos_stub().deserialize(buffer, {
			.copy_memory = options.eager_dos_stub_data_copy,
			.e_lfanew = dos_hdr.base_struct()->e_lfanew
		});

		instance.get_image_signature().deserialize(
			*buffer, options.allow_virtual_headers);
		if (auto err = validate(instance.get_image_signature()); err)
			result.warnings.add_error(err);

		auto& file_hdr = instance.get_file_header();
		auto& optional_hdr = instance.get_optional_header();
		file_hdr.deserialize(*buffer, options.allow_virtual_headers);
		optional_hdr.deserialize(*buffer, options.allow_virtual_headers);
		if (options.validate_size_of_optional_header)
		{
			if (auto err = validate_size_of_optional_header(
				file_hdr.base_struct()->size_of_optional_header,
				optional_hdr); err)
			{
				result.warnings.add_error(err);
			}
		}
		validate(optional_hdr, options.optional_header_validation,
			file_hdr.is_dll(), result.warnings);

		instance.get_data_directories().deserialize(*buffer,
			optional_hdr.get_number_of_rva_and_sizes(), options.allow_virtual_headers);

		if (options.validate_image_base)
		{
			if (auto err = validate_image_base(optional_hdr,
				instance.has_relocations()); err)
			{
				result.warnings.add_error(err);
			}
		}

		try
		{
			buffer->set_rpos(file_hdr.get_section_table_buffer_pos());
		}
		catch (...)
		{
			std::throw_with_nested(pe_error(section::section_errc::unable_to_read_section_table));
		}

		auto& section_tbl = instance.get_section_table();
		section_tbl.deserialize(*buffer, file_hdr.base_struct()->number_of_sections,
			options.allow_virtual_headers);

		const auto& section_headers = section_tbl.get_section_headers();
		if (options.validate_sections)
		{
			section::validate_section_headers(optional_hdr, section_headers, result.warnings);
		}

		if (options.load_section_data)
		{
			section::section_data_load_options load_opts{
				.section_alignment = optional_hdr.get_raw_section_alignment(),
				.copy_memory = options.eager_section_data_copy,
				.image_loaded_to_memory = options.image_loaded_to_memory,
				.image_start_buffer_pos = dos_hdr.base_struct().get_state().buffer_pos()
			};

			auto& sections = instance.get_section_data_list();
			for (auto it = section_headers.cbegin(),
				end = section_headers.cend(); it != end; ++it)
			{
				try
				{
					sections.emplace_back().deserialize(*it, buffer, load_opts);
				}
				catch (const pe_error& e)
				{
					result.warnings.add_error(e.code(),
						std::distance(section_headers.cbegin(), it));
				}
			}
		}

		if (options.load_overlay && !options.image_loaded_to_memory)
		{
			std::uint32_t last_section_offset = 0;
			for (auto it = section_headers.cbegin(),
				end = section_headers.cend(); it != end; ++it)
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

		if (options.validate_size_of_image)
		{
			const auto* last_section = section_headers.empty()
				? nullptr : &section_headers.back();
			if (auto err = validate_size_of_image(last_section, optional_hdr); err)
				result.warnings.add_error(err);
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
	}
	catch (...)
	{
		result.fatal_error = std::current_exception();
	}

	return result;
}

} //namespace pe_bliss::image