#include "pe_bliss2/image/byte_vector_from_va.h"

#include "buffers/input_buffer_stateful_wrapper.h"
#include "pe_bliss2/image/section_data_from_va.h"

namespace pe_bliss::image
{

packed_byte_vector byte_vector_from_rva(const image& instance, rva_type rva,
	std::uint32_t size, bool include_headers, bool allow_virtual_data)
{
	packed_byte_vector result;
	byte_vector_from_rva(instance, rva,
		size, result, include_headers, allow_virtual_data);
	return result;
}

void byte_vector_from_rva(const image& instance, rva_type rva,
	std::uint32_t size, packed_byte_vector& arr,
	bool include_headers, bool allow_virtual_data)
{
	auto buf = section_data_from_rva(instance, rva,
		include_headers, allow_virtual_data);
	buffers::input_buffer_stateful_wrapper_ref wrapper(*buf);
	arr.deserialize(wrapper, size, allow_virtual_data);
}

} //namespace pe_bliss::image
