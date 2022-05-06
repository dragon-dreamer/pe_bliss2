#include "pe_bliss2/section/section_table.h"

#include <algorithm>
#include <iterator>
#include <cstdint>
#include <system_error>

#include "buffers/input_buffer_interface.h"
#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/data_directories.h"
#include "pe_bliss2/file_header.h"
#include "pe_bliss2/optional_header.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/section/section_search.h"
#include "utilities/math.h"

namespace
{

struct section_table_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "section_table";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::section::section_table_errc;
		switch (static_cast<pe_bliss::section::section_table_errc>(ev))
		{
		case invalid_section_raw_size:
			return "Invalid section physical size";
		case invalid_section_virtual_size:
			return "Invalid section virtual size";
		case invalid_section_raw_address:
			return "Invalid section physical address";
		case invalid_section_raw_size_alignment:
			return "Invalid section physical size alignment";
		case invalid_section_raw_address_alignment:
			return "Invalid section physical address alignment";
		case raw_section_size_overflow:
			return "Raw section size overflows";
		case virtual_section_size_overflow:
			return "Virtual section size overflows";
		case invalid_section_virtual_address_alignment:
			return "Invalid section virtual address alignment";
		case virtual_gap_between_sections:
			return "Virtual gap between sections";
		case invalid_section_low_alignment:
			return "Invalid section alignment for low-aligned image";
		case virtual_gap_between_headers_and_first_section:
			return "Virtual gap between headers and first section";
		case invalid_size_of_image:
			return "Invalid size of image";
		case unable_to_read_section_table:
			return "Unable to read section table";
		case invalid_section_offset:
			return "Invalid offset within section";
		case unable_to_read_section_data:
			return "Unable to read section data";
		default:
			return {};
		}
	}
};

const section_table_error_category section_table_error_category_instance;

} //namespace

namespace pe_bliss::section
{

std::error_code make_error_code(section_table_errc e) noexcept
{
	return { static_cast<int>(e), section_table_error_category_instance };
}

void section_table::deserialize(buffers::input_buffer_interface& buf,
	const file_header& fh, const optional_header& oh, bool allow_virtual_memory)
{
	headers_.clear();
	buffer_pos_ = 0;

	auto number_of_sections = fh.base_struct()->number_of_sections;
	if (!number_of_sections)
		return;

	std::int32_t size_of_optional_header = fh.base_struct()->size_of_optional_header;
	std::int32_t full_optional_header_size = oh.get_size_of_structure()
		+ oh.get_number_of_rva_and_sizes() * data_directories::base_struct_type::packed_size;
	if (size_of_optional_header < full_optional_header_size)
		throw pe_error(section_table_errc::unable_to_read_section_table);

	buf.advance_rpos(size_of_optional_header - full_optional_header_size);
	buffer_pos_ = buf.rpos() + buf.absolute_offset();

	bool is_virtual = false;
	for (std::uint16_t i = 0; i != number_of_sections; ++i)
	{
		auto& header = headers_.emplace_back();
		if (!is_virtual)
		{
			header.deserialize(buf, allow_virtual_memory);
			is_virtual = header.base_struct().is_virtual();
		}
		else
		{
			header.base_struct().set_physical_size(0u);
		}
	}
}

void section_table::serialize(buffers::output_buffer_interface& buf,
	bool write_virtual_part) const
{
	for (const auto& header : headers_)
		header.serialize(buf, write_virtual_part);
}

pe_error_wrapper section_table::validate_section_header(
	const optional_header& oh, header_list::const_iterator header) const noexcept
{
	auto section_alignment = oh.get_raw_section_alignment();
	if (!header->check_raw_size(section_alignment))
		return section_table_errc::invalid_section_raw_size;

	if (!header->check_virtual_size())
		return section_table_errc::invalid_section_virtual_size;

	if (!header->check_raw_address())
		return section_table_errc::invalid_section_raw_address;

	auto file_alignment = oh.get_raw_file_alignment();
	if (!header->check_raw_size_alignment(section_alignment, file_alignment,
		header == std::prev(headers_.cend())))
	{
		return section_table_errc::invalid_section_raw_size_alignment;
	}

	if (!header->check_raw_address_alignment(file_alignment))
		return section_table_errc::invalid_section_raw_address_alignment;

	if (!header->check_raw_size_bounds(section_alignment))
		return section_table_errc::raw_section_size_overflow;

	if (!header->check_virtual_size_bounds(section_alignment))
		return section_table_errc::virtual_section_size_overflow;

	if (!header->check_virtual_address_alignment(section_alignment))
		return section_table_errc::invalid_section_virtual_address_alignment;

	if (header != headers_.cbegin())
	{
		auto prev = std::prev(header);
		auto next_virtual_address = prev->get_virtual_size(section_alignment);
		if (!utilities::math::add_if_safe(next_virtual_address, prev->base_struct()->virtual_address))
			return section_table_errc::invalid_section_virtual_size;
		if (next_virtual_address != header->base_struct()->virtual_address)
		{
			return section_table_errc::virtual_gap_between_sections;
		}
	}
	else
	{
		auto aligned_size_of_headers = oh.get_raw_size_of_headers();
		if (!utilities::math::align_up_if_safe(aligned_size_of_headers, section_alignment))
			return section_table_errc::invalid_section_virtual_size;

		if (header->base_struct()->virtual_address != aligned_size_of_headers)
			return section_table_errc::virtual_gap_between_headers_and_first_section;
	}

	if (oh.is_low_alignment() && !header->check_low_alignment())
		return section_table_errc::invalid_section_low_alignment;

	return {};
}

pe_error_wrapper section_table::validate_section_headers(
	const optional_header& oh) const noexcept
{
	pe_error_wrapper result;
	for (auto it = headers_.cbegin(), end = headers_.cend(); it != end; ++it)
	{
		if ((result = validate_section_header(oh, it)))
			break;
	}
	return result;
}

pe_error_wrapper section_table::validate_size_of_image(
	const optional_header& oh) const noexcept
{
	if (headers_.empty())
		return {};

	auto real_size_of_image = headers_.back().base_struct()->virtual_address;
	if (!utilities::math::add_if_safe(real_size_of_image,
		headers_.back().get_virtual_size(oh.get_raw_section_alignment())))
	{
		return section_table_errc::invalid_size_of_image;
	}
	if (oh.get_raw_size_of_image() != real_size_of_image)
		return section_table_errc::invalid_size_of_image;
	return {};
}

section_table::header_list::const_iterator section_table::by_rva(rva_type rva,
	std::uint32_t section_alignment, std::uint32_t data_size) const
{
	return std::find_if(std::cbegin(headers_), std::cend(headers_),
		section::by_rva(rva, section_alignment, data_size));
}

section_table::header_list::iterator section_table::by_rva(rva_type rva,
	std::uint32_t section_alignment, std::uint32_t data_size)
{
	return std::find_if(std::begin(headers_), std::end(headers_),
		section::by_rva(rva, section_alignment, data_size));
}

section_table::header_list::const_iterator section_table::by_raw_offset(
	std::uint32_t raw_offset,
	std::uint32_t section_alignment, std::uint32_t data_size) const
{
	return std::find_if(std::cbegin(headers_), std::cend(headers_),
		section::by_raw_offset(raw_offset, section_alignment, data_size));
}

section_table::header_list::iterator section_table::by_raw_offset(
	std::uint32_t raw_offset,
	std::uint32_t section_alignment, std::uint32_t data_size)
{
	return std::find_if(std::begin(headers_), std::end(headers_),
		section::by_raw_offset(raw_offset, section_alignment, data_size));
}

section_table::header_list::const_iterator section_table::by_reference(
	const section_header& header) const noexcept
{
	return std::find_if(std::cbegin(headers_), std::cend(headers_),
		section::by_pointer(&header));
}

section_table::header_list::iterator section_table::by_reference(
	section_header& header) noexcept
{
	return std::find_if(std::begin(headers_), std::end(headers_),
		section::by_pointer(&header));
}

} //namespace pe_bliss::section
