#include "pe_bliss2/image/rva_file_offset_converter.h"

#include "buffers/input_buffer_interface.h"

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_section_search.h"

#include "utilities/math.h"
#include "utilities/safe_uint.h"

namespace pe_bliss::image
{

std::uint32_t file_offset_to_rva(const image& instance, std::uint32_t offset)
{
	if (offset < instance.get_optional_header().get_raw_size_of_headers())
		return offset;

	auto it = section_from_file_offset(instance, offset, 1u).first;
	if (it == instance.get_section_table().get_section_headers().cend())
		throw pe_error(utilities::generic_errc::buffer_overrun);

	std::uint32_t result = offset - it->get_pointer_to_raw_data();
	if (result >= it->get_virtual_size(
		instance.get_optional_header().get_raw_section_alignment()))
	{
		throw pe_error(utilities::generic_errc::buffer_overrun);
	}

	if (!utilities::math::add_if_safe(result, it->get_rva()))
		throw pe_error(utilities::generic_errc::integer_overflow);

	return result;
}

std::uint32_t rva_to_file_offset(const image& instance, rva_type rva)
{
	if (rva < instance.get_optional_header().get_raw_size_of_headers())
		return rva;

	auto it = section_from_rva(instance, rva, 1u).first;
	if (it == instance.get_section_table().get_section_headers().cend())
		throw pe_error(utilities::generic_errc::buffer_overrun);

	std::uint32_t result = rva - it->get_rva();
	if (result >= it->get_raw_size(
		instance.get_optional_header().get_raw_section_alignment()))
	{
		throw pe_error(utilities::generic_errc::buffer_overrun);
	}

	if (!utilities::math::add_if_safe(result, it->get_pointer_to_raw_data()))
		throw pe_error(utilities::generic_errc::integer_overflow);

	return result;
}

rva_type absolute_offset_to_rva(const image& instance,
	const buffers::input_buffer_interface& buf)
{
	utilities::safe_uint<std::uint32_t> offset;
	offset += buf.absolute_offset();
	return file_offset_to_rva(instance, offset.value());
}

} //namespace pe_bliss::image
