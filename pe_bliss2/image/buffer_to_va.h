#pragma once

#include <cstdint>

#include "pe_bliss2/pe_types.h"

namespace buffers
{
class ref_buffer;
} //namespace buffers

namespace pe_bliss::image
{

class image;

rva_type buffer_to_rva(image& instance, rva_type rva,
	const buffers::ref_buffer& buf, bool include_headers = false,
	bool write_virtual_data = false);
std::uint32_t buffer_to_va(image& instance, std::uint32_t va,
	const buffers::ref_buffer& buf, bool include_headers = false,
	bool write_virtual_data = false);
std::uint64_t buffer_to_va(image& instance, std::uint64_t va,
	const buffers::ref_buffer& buf, bool include_headers = false,
	bool write_virtual_data = false);

rva_type buffer_to_file_offset(image& instance,
	const buffers::ref_buffer& buf, bool include_headers = false,
	bool write_virtual_data = false);

} //namespace pe_bliss::image
