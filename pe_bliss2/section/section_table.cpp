#include "pe_bliss2/section/section_table.h"

#include <algorithm>
#include <iterator>
#include <cstdint>

#include "buffers/input_buffer_interface.h"
#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/section/section_search.h"

namespace pe_bliss::section
{

void section_table::deserialize(buffers::input_buffer_interface& buf,
	std::uint16_t number_of_sections, bool allow_virtual_memory)
{
	headers_.clear();
	buffer_pos_ = 0;

	if (!number_of_sections)
		return;

	buffer_pos_ = buf.rpos();

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
