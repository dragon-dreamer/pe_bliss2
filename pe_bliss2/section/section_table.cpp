#include "pe_bliss2/section/section_table.h"

#include <algorithm>
#include <iterator>
#include <cstdint>

#include "buffers/input_buffer_interface.h"
#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/core/optional_header.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/section/section_errc.h"
#include "pe_bliss2/section/section_search.h"
#include "utilities/math.h"

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

pe_error_wrapper section_table::validate_section_header(
	const core::optional_header& oh, header_list::const_iterator header) const noexcept
{
	auto section_alignment = oh.get_raw_section_alignment();
	if (!header->check_raw_size(section_alignment))
		return section_errc::invalid_section_raw_size;

	if (!header->check_virtual_size())
		return section_errc::invalid_section_virtual_size;

	if (!header->check_raw_address())
		return section_errc::invalid_section_raw_address;

	auto file_alignment = oh.get_raw_file_alignment();
	if (!header->check_raw_size_alignment(section_alignment, file_alignment,
		header == std::prev(headers_.cend())))
	{
		return section_errc::invalid_section_raw_size_alignment;
	}

	if (!header->check_raw_address_alignment(file_alignment))
		return section_errc::invalid_section_raw_address_alignment;

	if (!header->check_raw_size_bounds(section_alignment))
		return section_errc::raw_section_size_overflow;

	if (!header->check_virtual_size_bounds(section_alignment))
		return section_errc::virtual_section_size_overflow;

	if (!header->check_virtual_address_alignment(section_alignment))
		return section_errc::invalid_section_virtual_address_alignment;

	if (header != headers_.cbegin())
	{
		auto prev = std::prev(header);
		auto next_virtual_address = prev->get_virtual_size(section_alignment);
		if (!utilities::math::add_if_safe(next_virtual_address, prev->base_struct()->virtual_address))
			return section_errc::invalid_section_virtual_size;
		if (next_virtual_address != header->base_struct()->virtual_address)
		{
			return section_errc::virtual_gap_between_sections;
		}
	}
	else
	{
		auto aligned_size_of_headers = oh.get_raw_size_of_headers();
		if (!utilities::math::align_up_if_safe(aligned_size_of_headers, section_alignment))
			return section_errc::invalid_section_virtual_size;

		if (header->base_struct()->virtual_address != aligned_size_of_headers)
			return section_errc::virtual_gap_between_headers_and_first_section;
	}

	if (oh.is_low_alignment() && !header->check_low_alignment())
		return section_errc::invalid_section_low_alignment;

	return {};
}

pe_error_wrapper section_table::validate_section_headers(
	const core::optional_header& oh) const noexcept
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
	const core::optional_header& oh) const noexcept
{
	if (headers_.empty())
		return {};

	auto real_size_of_image = headers_.back().base_struct()->virtual_address;
	if (!utilities::math::add_if_safe(real_size_of_image,
		headers_.back().get_virtual_size(oh.get_raw_section_alignment())))
	{
		return section_errc::invalid_size_of_image;
	}
	if (oh.get_raw_size_of_image() != real_size_of_image)
		return section_errc::invalid_size_of_image;
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
