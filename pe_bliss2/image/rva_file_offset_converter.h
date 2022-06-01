#pragma once

#include <cstdint>

#include "pe_bliss2/pe_types.h"

#include "utilities/safe_uint.h"

namespace buffers
{
class input_buffer_interface;
} //namespace buffers

namespace pe_bliss::image
{

class image;

[[nodiscard]]
std::uint32_t file_offset_to_rva(const image& instance, std::uint32_t offset);
[[nodiscard]]
std::uint32_t rva_to_file_offset(const image& instance, rva_type rva);

template<typename PackedStruct>
[[nodiscard]]
rva_type absolute_offset_to_rva(const image& instance,
	const PackedStruct& obj)
{
	utilities::safe_uint<std::uint32_t> offset;
	offset += obj.get_state().absolute_offset();
	return file_offset_to_rva(instance, offset.value());
}

[[nodiscard]]
rva_type absolute_offset_to_rva(const image& instance,
	const buffers::input_buffer_interface& buf);

} //namespace pe_bliss::image
