#include "pe_bliss2/core/optional_header_errc.h"

#include <string>
#include <system_error>

namespace
{

struct optional_header_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "optional_header";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::core::optional_header_errc;
		switch (static_cast<pe_bliss::core::optional_header_errc>(ev))
		{
		case invalid_pe_magic:
			return "Invalid PE magic number";
		case invalid_address_of_entry_point:
			return "Invalid address of entry point";
		case unaligned_image_base:
			return "Unaligned image base";
		case too_large_image_base:
			return "Too large image base for image with no relocations";
		case incorrect_section_alignment:
			return "Invalid section alignment";
		case incorrect_file_alignment:
			return "Invalid file alignment";
		case file_alignment_out_of_range:
			return "Image file alignment is out of range";
		case section_alignment_out_of_range:
			return "Image section alignment is out of range";
		case too_low_subsystem_version:
			return "Subsystem version is too low";
		case invalid_size_of_heap:
			return "Invalid size of heap commit/reserve";
		case invalid_size_of_stack:
			return "Invalid size of stack commit/reserve";
		case invalid_size_of_headers:
			return "Invalid size of headers";
		case no_base_of_data_field:
			return "PE64 optional header does not have BaseOfData field";
		case unable_to_read_optional_header:
			return "Unable to read optional header";
		case invalid_size_of_optional_header:
			return "Invalid size of optional header";
		case invalid_size_of_image:
			return "Invalid size of image";
		default:
			return {};
		}
	}
};

const optional_header_error_category optional_header_error_category_instance;

} //namespace

namespace pe_bliss::core
{
std::error_code make_error_code(optional_header_errc e) noexcept
{
	return { static_cast<int>(e), optional_header_error_category_instance };
}
} //namespace pe_bliss::core
