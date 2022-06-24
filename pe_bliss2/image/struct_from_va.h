#pragma once

#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/pe_types.h"

namespace pe_bliss::image
{

class image;

template<detail::standard_layout T>
[[nodiscard]]
packed_struct<T> struct_from_rva(const image& instance, rva_type rva,
	bool include_headers = false, bool allow_virtual_data = false)
{
	packed_struct<T> value{};
	auto buf = section_data_from_rva(instance, rva, include_headers);
	value.deserialize(*buf, allow_virtual_data);
	return value;
}

template<detail::standard_layout T, detail::executable_pointer Va>
[[nodiscard]]
packed_struct<T> struct_from_va(const image& instance, Va va,
	bool include_headers = false, bool allow_virtual_data = false)
{
	packed_struct<T> value{};
	auto buf = section_data_from_va(instance, va, include_headers);
	value.deserialize(*buf, allow_virtual_data);
	return value;
}

template<detail::standard_layout T>
packed_struct<T>& struct_from_rva(const image& instance,
	rva_type rva, packed_struct<T>& value,
	bool include_headers = false, bool allow_virtual_data = false)
{
	auto buf = section_data_from_rva(instance, rva, include_headers);
	value.deserialize(*buf, allow_virtual_data);
	return value;
}

template<detail::executable_pointer Va, detail::standard_layout T>
packed_struct<T>& struct_from_va(const image& instance,
	Va va, packed_struct<T>& value,
	bool include_headers = false, bool allow_virtual_data = false)
{
	auto buf = section_data_from_va(instance, va, include_headers);
	value.deserialize(*buf, allow_virtual_data);
	return value;
}

} //namespace pe_bliss::image