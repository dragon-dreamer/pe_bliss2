#include "pe_bliss2/image/section_data_length_from_va.h"

#include <iterator>

#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_section_search.h"

namespace pe_bliss::image
{

std::uint32_t section_data_length_from_rva(const image& instance,
	rva_type rva, bool include_headers, bool allow_virtual_data)
{
	if (include_headers && rva <= instance.get_full_headers_buffer().size())
	{
		return static_cast<std::uint32_t>(
			instance.get_full_headers_buffer().size()) - rva;
	}

	auto [header_it, data_it] = section_from_rva(instance, rva, 1u);
	if (data_it == std::end(instance.get_section_data_list()))
		throw pe_error(image_errc::section_data_does_not_exist);

	std::uint32_t data_offset = rva - header_it->get_rva();
	if (allow_virtual_data)
	{
		return header_it->get_virtual_size(
			instance.get_optional_header().get_raw_section_alignment())
			- data_offset;
	}

	if (data_offset >= data_it->size())
		return 0u;

	return static_cast<std::uint32_t>(data_it->size()) - data_offset;
}

std::uint32_t section_data_length_from_va(const image& instance,
	std::uint32_t va, bool include_headers, bool allow_virtual_data)
{
	return section_data_length_from_rva(instance,
		address_converter(instance).va_to_rva(va),
		include_headers, allow_virtual_data);
}

std::uint32_t section_data_length_from_va(const image& instance,
	std::uint64_t va, bool include_headers, bool allow_virtual_data)
{
	return section_data_length_from_rva(instance,
		address_converter(instance).va_to_rva(va),
		include_headers, allow_virtual_data);
}

} //namespace pe_bliss::image
