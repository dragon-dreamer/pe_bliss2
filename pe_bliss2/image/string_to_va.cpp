#include "pe_bliss2/image/string_to_va.h"

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/rva_file_offset_converter.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/packed_c_string.h"
#include "pe_bliss2/packed_utf16_string.h"

namespace pe_bliss::image
{

template<packed_string_type PackedString>
rva_type string_to_file_offset(image& instance, const PackedString& str,
	bool include_headers, bool write_virtual_part)
{
	return string_to_rva(instance, absolute_offset_to_rva(instance, str),
		str, include_headers, write_virtual_part);
}

template<packed_string_type PackedString>
rva_type string_to_rva(image& instance, rva_type rva, const PackedString& str,
	bool include_headers, bool write_virtual_part)
{
	if (str.value().empty() && str.is_virtual() && !write_virtual_part)
		return rva;

	auto buf = section_data_from_rva(instance, rva, include_headers);
	return rva + static_cast<rva_type>(
		str.serialize(buf.data(), buf.size_bytes(), write_virtual_part));
}

template<packed_string_type PackedString>
std::uint32_t string_to_va(image& instance, std::uint32_t va,
	const PackedString& str, bool include_headers, bool write_virtual_part)
{
	if (str.value().empty() && str.is_virtual() && !write_virtual_part)
		return va;

	auto buf = section_data_from_va(instance, va, include_headers);
	return va + static_cast<std::uint32_t>(
		str.serialize(buf.data(), buf.size_bytes(), write_virtual_part));
}

template<packed_string_type PackedString>
std::uint64_t string_to_va(image& instance, std::uint64_t va,
	const PackedString& str, bool include_headers, bool write_virtual_part)
{
	if (str.value().empty() && str.is_virtual() && !write_virtual_part)
		return va;

	auto buf = section_data_from_va(instance, va, include_headers);
	return va + static_cast<std::uint64_t>(
		str.serialize(buf.data(), buf.size_bytes(), write_virtual_part));
}

template rva_type string_to_rva<packed_c_string>(
	image& instance, rva_type rva, const packed_c_string& str,
	bool include_headers, bool write_virtual_part);
template std::uint32_t string_to_va<packed_c_string>(
	image& instance, std::uint32_t va, const packed_c_string& str,
	bool include_headers, bool write_virtual_part);
template std::uint64_t string_to_va<packed_c_string>(
	image& instance, std::uint64_t va, const packed_c_string& str,
	bool include_headers, bool write_virtual_part);
template rva_type string_to_file_offset<packed_c_string>(
	image& instance, const packed_c_string& str,
	bool include_headers, bool write_virtual_part);

template rva_type string_to_rva<packed_utf16_string>(
	image& instance, rva_type rva, const packed_utf16_string& str,
	bool include_headers, bool write_virtual_part);
template std::uint32_t string_to_va<packed_utf16_string>(
	image& instance, std::uint32_t va, const packed_utf16_string& str,
	bool include_headers, bool write_virtual_part);
template std::uint64_t string_to_va<packed_utf16_string>(
	image& instance, std::uint64_t va, const packed_utf16_string& str,
	bool include_headers, bool write_virtual_part);
template rva_type string_to_file_offset<packed_utf16_string>(
	image& instance, const packed_utf16_string& str,
	bool include_headers, bool write_virtual_part);

} //namespace pe_bliss::image
