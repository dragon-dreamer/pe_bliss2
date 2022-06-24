#pragma once

#include <cstddef>
#include <cstdint>

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/packed_byte_array.h"
#include "pe_bliss2/pe_types.h"

namespace pe_bliss::image
{

template<std::size_t MaxSize>
[[nodiscard]]
packed_byte_array<MaxSize> byte_array_from_rva(
	const image& instance, rva_type rva,
	std::uint32_t size, bool include_headers, bool allow_virtual_data)
{
	packed_byte_array<MaxSize> result;
	byte_array_from_rva(rva, result, size,
		include_headers, allow_virtual_data);
	return result;
}

template<std::size_t MaxSize>
void byte_array_from_rva(const image& instance, rva_type rva,
	packed_byte_array<MaxSize>& arr, std::uint32_t size,
	bool include_headers, bool allow_virtual_data)
{
	auto buf = section_data_from_rva(instance, rva, include_headers);
	arr.deserialize(*buf, size, allow_virtual_data);
}

template<detail::executable_pointer Va, std::size_t MaxSize>
[[nodiscard]]
packed_byte_array<MaxSize> byte_array_from_va(const image& instance, Va va,
	std::uint32_t size, bool include_headers, bool allow_virtual_data)
{
	return byte_array_from_rva(instance,
		address_converter(instance).va_to_rva(va),
		size, include_headers, allow_virtual_data);
}

template<detail::executable_pointer Va, std::size_t MaxSize>
void byte_array_from_va(const image& instance, Va va,
	packed_byte_array<MaxSize>& arr, std::uint32_t size,
	bool include_headers, bool allow_virtual_data)
{
	byte_array_from_rva(instance,
		address_converter(instance).va_to_rva(va), arr, size,
		include_headers, allow_virtual_data);
}

} //namespace pe_bliss::image
