#include "pe_bliss2/image/image_builder.h"

#include <cstdint>
#include <cstddef>
#include <system_error>

#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/image/image.h"
#include "utilities/safe_uint.h"

namespace
{

struct image_builder_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "image_builder";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::image::image_builder_errc;
		switch (static_cast<pe_bliss::image::image_builder_errc>(ev))
		{
		case inconsistent_section_headers_and_data:
			return "Inconsistent image section header and section data count";
		case invalid_section_table_offset:
			return "Invalid section table offset";
		default:
			return {};
		}
	}
};

const image_builder_error_category image_builder_error_category_instance;

} //namespace

namespace pe_bliss::image
{

std::error_code make_error_code(image_builder_errc e) noexcept
{
	return { static_cast<int>(e), image_builder_error_category_instance };
}

void image_builder::build(const image& instance, buffers::output_buffer_interface& buffer,
	const image_builder_options& options)
{
	const auto& section_tbl = instance.get_section_table();
	const auto& sections = instance.get_section_data_list();
	if (section_tbl.get_section_headers().size() != sections.size())
		throw pe_error(image_builder_errc::inconsistent_section_headers_and_data);

	const auto& dos_hdr = instance.get_dos_header().get_descriptor();
	const auto& image_signature = instance.get_image_signature();
	const auto& file_header = instance.get_file_header();
	const auto& optional_header = instance.get_optional_header();
	const auto& data_directories = instance.get_data_directories();

	auto buffer_start_pos = buffer.wpos();
	utilities::safe_uint<std::size_t> section_table_offset = dos_hdr->e_lfanew;
	try
	{
		if (!sections.empty())
		{
			section_table_offset += core::image_signature::descriptor_type::packed_size;
			section_table_offset += core::file_header::descriptor_type::packed_size;
			section_table_offset += file_header.get_descriptor()->size_of_optional_header;
			section_table_offset += buffer_start_pos;

			auto section_hdr = section_tbl.get_section_headers().cbegin();
			for (const auto& section : sections)
			{
				utilities::safe_uint<std::size_t> section_last_raw_address
					= section_hdr->get_pointer_to_raw_data();
				section_last_raw_address += section.size();
				section_last_raw_address += buffer_start_pos;
				++section_hdr;
			}
		}
	}
	catch (...)
	{
		std::throw_with_nested(pe_error(image_builder_errc::invalid_section_table_offset));
	}

	instance.get_dos_header().serialize(buffer, options.write_structure_virtual_parts);

	instance.get_dos_stub().serialize(buffer);
	if (buffer.wpos() - buffer_start_pos != dos_hdr->e_lfanew)
		buffer.set_wpos(buffer_start_pos + dos_hdr->e_lfanew);

	image_signature.serialize(buffer, options.write_structure_virtual_parts);
	file_header.serialize(buffer, options.write_structure_virtual_parts);
	optional_header.serialize(buffer, options.write_structure_virtual_parts);
	data_directories.serialize(buffer, options.write_structure_virtual_parts);

	const auto& full_headers = instance.get_full_headers_buffer();
	if (options.fill_full_headers_data_gaps)
	{
		auto written_size = buffer.wpos() - buffer_start_pos;
		if (full_headers.size() > written_size)
			full_headers.serialize_until(buffer, written_size);
	}

	if (sections.empty())
		return;

	buffer.set_wpos(section_table_offset.value());
	section_tbl.serialize(buffer, options.write_structure_virtual_parts);

	auto section_hdr = section_tbl.get_section_headers().cbegin();
	for (const auto& section : sections)
	{
		//Checked for overflow earlier
		buffer.set_wpos(section_hdr->get_pointer_to_raw_data()
			+ buffer_start_pos);
		section.serialize(buffer);
		++section_hdr;
	}
}

} //namespace pe_bliss::image
