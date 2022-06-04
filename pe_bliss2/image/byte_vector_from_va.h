#pragma once

#include <cstdint>

#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/packed_byte_vector.h"
#include "pe_bliss2/pe_types.h"

namespace pe_bliss::image
{

class image;

[[nodiscard]]
packed_byte_vector byte_vector_from_rva(const image& instance, rva_type rva,
	std::uint32_t size, bool include_headers, bool allow_virtual_data);
void byte_vector_from_rva(const image& instance,
	rva_type rva, packed_byte_vector& arr,
	std::uint32_t size, bool include_headers, bool allow_virtual_data);

template<detail::executable_pointer Va>
[[nodiscard]]
packed_byte_vector byte_vector_from_va(const image& instance, Va va,
	std::uint32_t size, bool include_headers, bool allow_virtual_data)
{
	return byte_vector_from_rva(instance,
		address_converter(instance).va_to_rva(va),
		size, include_headers, allow_virtual_data);
}

template<detail::executable_pointer Va>
void byte_vector_from_va(const image& instance, Va va,
	packed_byte_vector& arr, std::uint32_t size,
	bool include_headers, bool allow_virtual_data)
{
	byte_vector_from_rva(instance,
		address_converter(instance).va_to_rva(va), arr, size,
		include_headers, allow_virtual_data);
}

} //namespace pe_bliss::image
