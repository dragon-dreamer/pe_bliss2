#include "pe_bliss2/section/section_header_validator.h"

#include <iterator>

#include "pe_bliss2/core/optional_header.h"
#include "pe_bliss2/core/optional_header_errc.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/section/section_errc.h"
#include "pe_bliss2/section/section_header.h"
#include "pe_bliss2/section/section_header_validator.h"
#include "pe_bliss2/section/section_table.h"

#include "utilities/math.h"

namespace pe_bliss::section
{

void validate_section_header(const core::optional_header& oh,
	const section_header& header, std::size_t section_index,
	error_list& errors)
{
	auto section_alignment = oh.get_raw_section_alignment();
	if (auto err = validate_raw_size(header, section_alignment); err)
		errors.add_error(err, section_index);

	if (auto err = validate_virtual_size(header); err)
		errors.add_error(err, section_index);

	if (auto err = validate_raw_address(header); err)
		errors.add_error(err, section_index);

	auto file_alignment = oh.get_raw_file_alignment();
	if (auto err = validate_raw_address_alignment(header, file_alignment); err)
		errors.add_error(err, section_index);

	if (auto err = validate_raw_size_bounds(header, section_alignment); err)
		errors.add_error(err, section_index);

	if (auto err = validate_virtual_size_bounds(header, section_alignment); err)
		errors.add_error(err, section_index);

	if (auto err = validate_virtual_address_alignment(header, section_alignment); err)
		errors.add_error(err, section_index);

	if (oh.is_low_alignment() && !header.is_low_alignment())
		errors.add_error(section_errc::invalid_section_low_alignment, section_index);
}

void validate_section_header(const core::optional_header& oh,
	section_table::header_list::const_iterator header_it,
	const section_table::header_list& headers,
	error_list& errors)
{
	auto section_index = std::distance(std::cbegin(headers), header_it);
	const auto& header = *header_it;
	validate_section_header(oh, header, section_index, errors);

	auto section_alignment = oh.get_raw_section_alignment();
	auto file_alignment = oh.get_raw_file_alignment();
	if (auto err = validate_raw_size_alignment(header,
		section_alignment, file_alignment,
		header_it == std::prev(std::cend(headers))); err)
	{
		errors.add_error(err, section_index);
	}

	if (header_it != std::cbegin(headers))
	{
		auto prev = std::prev(header_it);
		auto next_virtual_address = prev->get_virtual_size(section_alignment);
		if (!utilities::math::add_if_safe(next_virtual_address, prev->get_rva()))
		{
			errors.add_error(section_errc::invalid_section_virtual_size,
				section_index - 1);
		}
		else if (next_virtual_address != header.get_rva())
		{
			errors.add_error(section_errc::virtual_gap_between_sections,
				section_index);
		}
	}
	else if (oh.get_raw_size_of_headers() > header.get_rva())
	{
		errors.add_error(core::optional_header_errc::invalid_size_of_headers);
	}
}

void validate_section_headers(const core::optional_header& oh,
	const section_table::header_list& headers,
	error_list& errors)
{
	for (auto it = std::cbegin(headers), end = std::cend(headers); it != end; ++it)
		validate_section_header(oh, it, headers, errors);
}

} //namespace pe_bliss::section
