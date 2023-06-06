#include "pe_bliss2/image/section_data_length_from_va.h"

#include <iterator>

#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_errc.h"
#include "pe_bliss2/image/image_section_search.h"

namespace pe_bliss::image
{

std::uint32_t section_data_length_from_rva(const image& instance,
	rva_type rva, bool include_headers, bool allow_virtual_data)
{
	if (rva <= instance.get_full_headers_buffer().size())
	{
		if (!include_headers)
			throw pe_error(image_errc::section_data_does_not_exist);

		auto size = instance.get_full_headers_buffer().size();
		if (!allow_virtual_data)
			size -= instance.get_full_headers_buffer().virtual_size();
		return static_cast<std::uint32_t>(size - rva);
	}

	auto [header_it, data_it] = section_from_rva(instance, rva, 1u);
	if (data_it == std::end(instance.get_section_data_list()))
	{
		auto [empty_header_it, empty_data_it] = section_from_rva(instance, rva, 0u);
		if (empty_data_it == std::end(instance.get_section_data_list()))
			throw pe_error(image_errc::section_data_does_not_exist);

		return 0u;
	}

	std::uint32_t data_offset = rva - header_it->get_rva();
	std::size_t real_size = data_it->size();
	if (!allow_virtual_data)
		real_size -= data_it->virtual_size();

	if (data_offset >= real_size)
		return 0u;

	return static_cast<std::uint32_t>(real_size - data_offset);
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
