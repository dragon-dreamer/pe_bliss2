#include "pe_bliss2/image/byte_vector_from_va.h"

#include "pe_bliss2/image/section_data_from_va.h"

namespace pe_bliss::image
{

packed_byte_vector byte_vector_from_rva(const image& instance, rva_type rva,
	std::uint32_t size, bool include_headers, bool allow_virtual_data)
{
	packed_byte_vector result;
	byte_vector_from_rva(instance, rva, result,
		size, include_headers, allow_virtual_data);
	return result;
}

void byte_vector_from_rva(const image& instance, rva_type rva,
	packed_byte_vector& arr, std::uint32_t size,
	bool include_headers, bool allow_virtual_data)
{
	auto buf = section_data_from_rva(instance, rva, size,
		include_headers, allow_virtual_data);
	arr.deserialize(*buf, size, allow_virtual_data);
}

} //namespace pe_bliss::image
