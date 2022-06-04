#pragma once

#include <cstdint>

#include "buffers/ref_buffer.h"

#include "pe_bliss2/pe_types.h"

namespace pe_bliss::image
{

class image;

rva_type buffer_to_rva(image& instance, rva_type rva,
	const buffers::ref_buffer& buf, bool include_headers = false);
std::uint32_t buffer_to_va(image& instance, std::uint32_t va,
	const buffers::ref_buffer& buf, bool include_headers = false);
std::uint64_t buffer_to_va(image& instance, std::uint64_t va,
	const buffers::ref_buffer& buf, bool include_headers = false);

rva_type buffer_to_file_offset(image& instance,
	const buffers::ref_buffer& buf, bool include_headers = false);

} //namespace pe_bliss::image
