#pragma once

#include <cstdint>

#include "pe_bliss2/pe_types.h"

namespace pe_bliss::image
{

class image;

[[nodiscard]]
std::uint32_t section_data_length_from_rva(
	const image& instance, rva_type rva,
	bool include_headers = false, bool allow_virtual_data = false);
[[nodiscard]]
std::uint32_t section_data_length_from_va(
	const image& instance, std::uint32_t va,
	bool include_headers = false, bool allow_virtual_data = false);
[[nodiscard]]
std::uint32_t section_data_length_from_va(
	const image& instance, std::uint64_t va,
	bool include_headers = false, bool allow_virtual_data = false);

} //namespace pe_bliss::image
