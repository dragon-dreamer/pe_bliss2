#pragma once

#include <cstdint>

#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/image/rva_file_offset_converter.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/pe_types.h"

#include "utilities/safe_uint.h"

namespace pe_bliss::image
{

class image;

template<detail::standard_layout T>
rva_type struct_to_rva(image& instance,
	rva_type rva, const packed_struct<T>& value,
	bool include_headers = false, bool write_virtual_part = false,
	bool cut_if_does_not_fit = false)
{
	if (!value.physical_size() && !write_virtual_part)
		return rva;

	auto buf = section_data_from_rva(instance, rva,
		static_cast<std::uint32_t>(
			write_virtual_part ? value.packed_size : value.physical_size()),
		include_headers, cut_if_does_not_fit);

	auto size = cut_if_does_not_fit
		? value.serialize_until(buf.data(), buf.size_bytes(), write_virtual_part)
		: value.serialize(buf.data(), buf.size_bytes(), write_virtual_part);

	return (utilities::safe_uint(rva) + size).value();
}

template<detail::executable_pointer Va, detail::standard_layout T>
Va struct_to_va(image& instance, Va va, const packed_struct<T>& value,
	bool include_headers = false, bool write_virtual_part = false,
	bool cut_if_does_not_fit = false)
{
	if (!value.physical_size() && !write_virtual_part)
		return va;

	auto buf = section_data_from_va(instance, va,
		static_cast<std::uint32_t>(
			write_virtual_part ? value.packed_size : value.physical_size()),
		include_headers, cut_if_does_not_fit);

	auto size = cut_if_does_not_fit
		? value.serialize_until(buf.data(), buf.size_bytes(), write_virtual_part)
		: value.serialize(buf.data(), buf.size_bytes(), write_virtual_part);

	return (utilities::safe_uint(va) + size).value();
}

template<detail::standard_layout T>
rva_type struct_to_rva(image& instance, rva_type rva,
	const T& value, bool include_headers = false,
	bool cut_if_does_not_fit = false)
{
	packed_struct wrapper(value);
	return struct_to_rva(instance, rva, wrapper,
		include_headers, true, cut_if_does_not_fit);
}

template<detail::executable_pointer Va, detail::standard_layout T>
Va struct_to_va(image& instance, Va va,
	const T& value, bool include_headers = false,
	bool cut_if_does_not_fit = false)
{
	packed_struct wrapper(value);
	return struct_to_va(instance, va, wrapper,
		include_headers, true, cut_if_does_not_fit);
}

template<detail::standard_layout T>
rva_type struct_to_file_offset(image& instance, const packed_struct<T>& value,
	bool include_headers = false, bool write_virtual_part = false)
{
	return struct_to_rva(instance, absolute_offset_to_rva(instance, value),
		value, include_headers, write_virtual_part);
}

} //namespace pe_bliss::image
