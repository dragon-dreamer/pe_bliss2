#pragma once

#include <cstdint>

#include "pe_bliss2/packed_string_type.h"
#include "pe_bliss2/pe_types.h"

namespace pe_bliss::image
{

class image;

template<packed_string_type PackedString>
rva_type string_to_rva(image& instance, rva_type rva,
	const PackedString& str, bool include_headers = false,
	bool write_virtual_part = false);
template<packed_string_type PackedString>
std::uint32_t string_to_va(image& instance, std::uint32_t va,
	const PackedString& str, bool include_headers = false,
	bool write_virtual_part = false);
template<packed_string_type PackedString>
std::uint64_t string_to_va(image& instance, std::uint64_t va,
	const PackedString& str, bool include_headers = false,
	bool write_virtual_part = false);

template<packed_string_type PackedString>
rva_type string_to_file_offset(image& instance,
	const PackedString& str, bool include_headers = false,
	bool write_virtual_part = false);

} //namespace pe_bliss::image
