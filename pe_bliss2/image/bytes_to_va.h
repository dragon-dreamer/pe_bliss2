#pragma once

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/rva_file_offset_converter.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/pe_types.h"

namespace pe_bliss::image
{

template<typename ArrayOrVector>
rva_type bytes_to_rva(image& instance, rva_type rva, const ArrayOrVector& arr,
	bool include_headers = false, bool write_virtual_part = false)
{
	if (!arr.data_size() || (!arr.physical_size() && !write_virtual_part))
		return rva;

	auto buf = section_data_from_rva(instance, rva, include_headers);
	return rva + static_cast<rva_type>(
		arr.serialize(buf.data(), buf.size_bytes(), write_virtual_part));
}

template<detail::executable_pointer Va, typename ArrayOrVector>
Va bytes_to_va(image& instance, Va va, const ArrayOrVector& arr,
	bool include_headers = false, bool write_virtual_part = false)
{
	if (!arr.data_size() || (!arr.physical_size() && !write_virtual_part))
		return va;

	auto buf = section_data_from_va(instance, va, include_headers);
	return va + static_cast<Va>(
		arr.serialize(buf.data(), buf.size_bytes(), write_virtual_part));
}

template<typename ArrayOrVector>
rva_type bytes_to_file_offset(image& instance, const ArrayOrVector& arr,
	bool include_headers = false, bool write_virtual_part = false)
{
	return bytes_to_rva(instance, absolute_offset_to_rva(instance, arr), arr,
		include_headers, write_virtual_part);
}

} //namespace pe_bliss::image
