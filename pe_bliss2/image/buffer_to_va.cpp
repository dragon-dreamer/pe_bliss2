#include "pe_bliss2/image/buffer_to_va.h"

#include "buffers/output_memory_ref_buffer.h"

#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/image/rva_file_offset_converter.h"

namespace pe_bliss::image
{

rva_type buffer_to_rva(image& instance,
	rva_type rva, const buffers::ref_buffer& buf, bool include_headers)
{
	if (buf.empty())
		return rva;

	auto data = section_data_from_rva(instance, rva, include_headers);
	buffers::output_memory_ref_buffer data_buffer(data);
	buf.serialize(data_buffer);
	return rva + static_cast<rva_type>(buf.size());
}

std::uint32_t buffer_to_va(image& instance,
	std::uint32_t va, const buffers::ref_buffer& buf, bool include_headers)
{
	if (buf.empty())
		return va;

	auto data = section_data_from_va(instance, va, include_headers);
	auto data_buffer = buffers::output_memory_ref_buffer(data);
	buf.serialize(data_buffer);
	return va + static_cast<std::uint32_t>(buf.size());
}

std::uint64_t buffer_to_va(image& instance,
	std::uint64_t va, const buffers::ref_buffer& buf, bool include_headers)
{
	if (buf.empty())
		return va;

	auto data = section_data_from_va(instance, va, include_headers);
	buffers::output_memory_ref_buffer data_buffer(data);
	buf.serialize(data_buffer);
	return va + static_cast<std::uint64_t>(buf.size());
}

rva_type buffer_to_file_offset(image& instance,
	const buffers::ref_buffer& buf, bool include_headers)
{
	return buffer_to_rva(instance,
		absolute_offset_to_rva(instance, *buf.data()),
		buf, include_headers);
}

} //namespace pe_bliss::image
