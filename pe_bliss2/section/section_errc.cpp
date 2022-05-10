#include "pe_bliss2/section/section_errc.h"

#include <string>
#include <system_error>

namespace
{

struct section_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "section";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::section::section_errc;
		switch (static_cast<pe_bliss::section::section_errc>(ev))
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

const section_error_category section_error_category_instance;

} //namespace

namespace pe_bliss::section
{

std::error_code make_error_code(section_errc e) noexcept
{
	return { static_cast<int>(e), section_error_category_instance };
}

} //namespace pe_bliss::section
