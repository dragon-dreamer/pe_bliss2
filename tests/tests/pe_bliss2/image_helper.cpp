#include "tests/tests/pe_bliss2/image_helper.h"

#include "buffers/input_container_buffer.h"
#include "buffers/input_virtual_buffer.h"
#include "pe_bliss2/dos/dos_header.h"
#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/core/image_signature.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/core/optional_header.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/section/section_header.h"

pe_bliss::image::image create_test_image(const test_image_options& options)
{
	using namespace pe_bliss;

	image::image instance;

	if (options.is_x64)
	{
		instance.get_optional_header().initialize_with<
			core::optional_header::optional_header_64_type>();
	}
	else
	{
		instance.get_optional_header().initialize_with<
			core::optional_header::optional_header_32_type>();
	}

	auto& dos_struct = instance.get_dos_header().get_descriptor();
	dos_struct->e_magic = dos::dos_header::mz_magic_value;
	dos_struct->e_lfanew = options.e_lfanew;

	instance.get_image_signature().set_signature(core::image_signature::pe_signature);
	instance.get_file_header().set_machine_type(options.is_x64
		? core::file_header::machine_type::amd64
		: core::file_header::machine_type::i386);
	instance.set_number_of_data_directories(options.number_of_data_directories);
	instance.get_section_table().get_section_headers().resize(options.sections.size());
	instance.get_section_data_list().resize(options.sections.size());
	instance.update_number_of_sections();

	instance.get_file_header().get_descriptor()->size_of_optional_header
		= static_cast<std::uint16_t>(
			instance.get_optional_header().get_size_of_structure()
			+ instance.get_data_directories().get_directories().size()
			* core::data_directories::directory_packed_size);

	instance.get_optional_header().set_raw_size_of_headers(
		static_cast<std::uint32_t>(options.e_lfanew
			+ core::image_signature::descriptor_type::packed_size
			+ core::file_header::descriptor_type::packed_size
			+ instance.get_file_header().get_descriptor()->size_of_optional_header
			+ options.sections.size()
			* section::section_header::descriptor_type::packed_size)
	);

	instance.get_optional_header().set_raw_image_base(options.image_base);
	instance.get_optional_header().set_raw_file_alignment(options.file_alignment);
	instance.get_optional_header().set_raw_section_alignment(options.section_alignment);

	auto section_headers_it = instance.get_section_table().get_section_headers().begin();
	auto section_data_it = instance.get_section_data_list().begin();
	std::uint32_t current_rva = options.start_section_rva;
	std::uint32_t current_raw_pos = options.start_section_raw_offset;
	for (auto [vsize, rsize] : options.sections)
	{
		section_headers_it->set_raw_size(rsize);
		section_headers_it->set_virtual_size(vsize);
		section_headers_it->set_rva(current_rva);
		current_rva += vsize;
		if (rsize)
		{
			section_headers_it->set_pointer_to_raw_data(current_raw_pos);
			current_raw_pos += rsize;
		}

		if (vsize > rsize)
		{
			auto container_buf = std::make_shared<buffers::input_container_buffer>();
			container_buf->get_container().resize(rsize);
			auto virtual_buf = std::make_shared<buffers::input_virtual_buffer>(
				container_buf, vsize - rsize);
			section_data_it->get_buffer().deserialize(virtual_buf, false);
		}
		else
		{
			section_data_it->copied_data().resize(rsize);
		}

		++section_headers_it;
		++section_data_it;
	}

	return instance;
}
