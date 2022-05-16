#include "pe_bliss2/image/image_builder.h"

#include <cstdint>
#include <cstddef>
#include <system_error>

#include "buffers/output_buffer_interface.h"

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

void image_builder::build(const image& instance, const image_builder_options& options,
	buffers::output_buffer_interface& buffer)
{
	const auto& section_tbl = instance.get_section_table();
	const auto& sections = instance.get_section_data_list();
	if (section_tbl.get_section_headers().size() != sections.size())
		throw pe_error(image_builder_errc::inconsistent_section_headers_and_data);

	auto buffer_start_pos = buffer.wpos();
	const auto& dos_hdr = instance.get_dos_header().base_struct();
	auto image_start_pos = dos_hdr.get_state().buffer_pos();
	dos_hdr.serialize(buffer, true);

	instance.get_dos_stub().serialize(buffer);
	instance.get_image_signature().serialize(buffer, options.write_structure_virtual_parts);
	instance.get_file_header().serialize(buffer, options.write_structure_virtual_parts);
	instance.get_optional_header().serialize(buffer, options.write_structure_virtual_parts);
	instance.get_data_directories().serialize(buffer, options.write_structure_virtual_parts);

	if (section_tbl.get_section_headers().empty())
		return;

	std::uint32_t section_table_offset = dos_hdr->e_lfanew;
	section_table_offset += core::image_signature::packed_struct_type::packed_size;
	section_table_offset += core::file_header::packed_struct_type::packed_size;
	section_table_offset += instance.get_file_header().base_struct()->size_of_optional_header;
	std::size_t section_table_pos = section_table_offset - image_start_pos + buffer_start_pos;

	const auto& full_headers = instance.get_full_headers_buffer();
	if (options.fill_full_headers_data_gaps)
	{
		auto wpos = buffer.wpos();
		if (wpos < section_table_pos && full_headers.size() > wpos)
			full_headers.serialize(buffer, wpos, section_table_pos - wpos);
	}

	buffer.set_wpos(section_table_pos);

	section_tbl.serialize(buffer, options.write_structure_virtual_parts);

	if (options.fill_full_headers_data_gaps)
	{
		auto wpos = buffer.wpos();
		if (full_headers.size() > wpos)
			full_headers.serialize(buffer, wpos);
	}

	auto section_hdr = section_tbl.get_section_headers().cbegin();
	for (const auto& section : sections)
	{
		buffer.set_wpos(section_hdr->get_pointer_to_raw_data()
			- image_start_pos + buffer_start_pos);
		section.serialize(buffer);
		++section_hdr;
	}
}

} //namespace pe_bliss::image
