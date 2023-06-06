#include "pe_bliss2/image/string_from_va.h"

#include "buffers/input_buffer_stateful_wrapper.h"
#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/packed_c_string.h"
#include "pe_bliss2/packed_utf16_string.h"

namespace pe_bliss::image
{

template<packed_string_type PackedString>
PackedString string_from_rva(const image& instance, rva_type rva,
	bool include_headers, bool allow_virtual_data)
{
	PackedString result;
	string_from_rva(instance, rva, result, include_headers, allow_virtual_data);
	return result;
}

template<packed_string_type PackedString>
void string_from_rva(const image& instance, rva_type rva, PackedString& str,
	bool include_headers, bool allow_virtual_data)
{
	auto buf = section_data_from_rva(instance, rva,
		include_headers, allow_virtual_data);
	buffers::input_buffer_stateful_wrapper_ref wrapper(*buf);
	str.deserialize(wrapper, allow_virtual_data);
}

template<packed_string_type PackedString>
PackedString string_from_va(const image& instance, std::uint32_t va,
	bool include_headers, bool allow_virtual_data)
{
	return string_from_rva<PackedString>(instance,
		address_converter(instance).va_to_rva(va),
		include_headers, allow_virtual_data);
}

template<packed_string_type PackedString>
void string_from_va(const image& instance, std::uint32_t va, PackedString& str,
	bool include_headers, bool allow_virtual_data)
{
	string_from_rva(instance, address_converter(instance).va_to_rva(va), str,
		include_headers, allow_virtual_data);
}

template<packed_string_type PackedString>
PackedString string_from_va(const image& instance, std::uint64_t va,
	bool include_headers, bool allow_virtual_data)
{
	return string_from_rva<PackedString>(instance,
		address_converter(instance).va_to_rva(va),
		include_headers, allow_virtual_data);
}

template<packed_string_type PackedString>
void string_from_va(const image& instance, std::uint64_t va, PackedString& str,
	bool include_headers, bool allow_virtual_data)
{
	string_from_rva(instance, address_converter(instance).va_to_rva(va), str,
		include_headers, allow_virtual_data);
}

template packed_c_string string_from_rva<packed_c_string>(
	const image& instance, rva_type rva,
	bool include_headers, bool allow_virtual_data);
template void string_from_rva<packed_c_string>(
	const image& instance, rva_type rva,
	packed_c_string& str, bool include_headers,
	bool allow_virtual_data);
template packed_c_string string_from_va<packed_c_string>(
	const image& instance, std::uint32_t va,
	bool include_headers, bool allow_virtual_data);
template void string_from_va<packed_c_string>(
	const image& instance, std::uint32_t va,
	packed_c_string& str, bool include_headers,
	bool allow_virtual_data);
template packed_c_string string_from_va<packed_c_string>(
	const image& instance, std::uint64_t va,
	bool include_headers, bool allow_virtual_data);
template void string_from_va<packed_c_string>(
	const image& instance, std::uint64_t va,
	packed_c_string& str, bool include_headers,
	bool allow_virtual_data);

template packed_utf16_string string_from_rva<packed_utf16_string>(
	const image& instance, rva_type rva,
	bool include_headers, bool allow_virtual_data);
template void string_from_rva<packed_utf16_string>(
	const image& instance, rva_type rva,
	packed_utf16_string& str, bool include_headers,
	bool allow_virtual_data);
template packed_utf16_string string_from_va<packed_utf16_string>(
	const image& instance, std::uint32_t va,
	bool include_headers, bool allow_virtual_data);
template void string_from_va<packed_utf16_string>(
	const image& instance, std::uint32_t va,
	packed_utf16_string& str, bool include_headers,
	bool allow_virtual_data);
template packed_utf16_string string_from_va<packed_utf16_string>(
	const image& instance, std::uint64_t va,
	bool include_headers, bool allow_virtual_data);
template void string_from_va<packed_utf16_string>(
	const image& instance, std::uint64_t va,
	packed_utf16_string& str, bool include_headers,
	bool allow_virtual_data);

} //namespace pe_bliss::image
