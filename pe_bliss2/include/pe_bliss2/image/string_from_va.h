#pragma once

#include <cstdint>

#include "pe_bliss2/packed_string_type.h"
#include "pe_bliss2/pe_types.h"

namespace pe_bliss::image
{

class image;

template<packed_string_type PackedString = packed_c_string>
[[nodiscard]]
PackedString string_from_rva(const image& instance, rva_type rva,
	bool include_headers = false, bool allow_virtual_data = false);
template<packed_string_type PackedString>
void string_from_rva(const image& instance, rva_type rva, PackedString& str,
	bool include_headers = false, bool allow_virtual_data = false);

template<packed_string_type PackedString = packed_c_string>
[[nodiscard]]
PackedString string_from_va(const image& instance, std::uint32_t va,
	bool include_headers = false, bool allow_virtual_data = false);
template<packed_string_type PackedString>
void string_from_va(const image& instance, std::uint32_t va, PackedString& str,
	bool include_headers = false, bool allow_virtual_data = false);

template<packed_string_type PackedString = packed_c_string>
[[nodiscard]]
PackedString string_from_va(const image& instance, std::uint64_t va,
	bool include_headers = false, bool allow_virtual_data = false);
template<packed_string_type PackedString>
void string_from_va(const image& instance, std::uint64_t va, PackedString& str,
	bool include_headers = false, bool allow_virtual_data = false);

} //namespace pe_bliss::image
