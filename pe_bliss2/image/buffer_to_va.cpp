#include "pe_bliss2/image/buffer_to_va.h"

#include "buffers/output_memory_ref_buffer.h"
#include "buffers/ref_buffer.h"

#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/image/rva_file_offset_converter.h"

namespace pe_bliss::image
{

rva_type buffer_to_rva(image& instance,
	rva_type rva, const buffers::ref_buffer& buf, bool include_headers,
	bool write_virtual_data)
{
	auto size = buf.size();
	if (!write_virtual_data)
		size -= buf.virtual_size();

	if (!size)
		return rva;

	auto data = section_data_from_rva(instance, rva, include_headers);
	buffers::output_memory_ref_buffer data_buffer(data);
	buf.serialize(data_buffer, write_virtual_data);
	return static_cast<rva_type>(rva + size);
}

std::uint32_t buffer_to_va(image& instance,
	std::uint32_t va, const buffers::ref_buffer& buf, bool include_headers,
	bool write_virtual_data)
{
	address_converter conv(instance);
	return conv.rva_to_va<std::uint32_t>(
		buffer_to_rva(instance, conv.va_to_rva(va), buf, include_headers,
			write_virtual_data));
}

std::uint64_t buffer_to_va(image& instance,
	std::uint64_t va, const buffers::ref_buffer& buf, bool include_headers,
	bool write_virtual_data)
{
	address_converter conv(instance);
	return conv.rva_to_va<std::uint64_t>(
		buffer_to_rva(instance, conv.va_to_rva(va), buf, include_headers,
			write_virtual_data));
}

rva_type buffer_to_file_offset(image& instance,
	const buffers::ref_buffer& buf, bool include_headers,
	bool write_virtual_data)
{
	return buffer_to_rva(instance,
		absolute_offset_to_rva(instance, *buf.data()),
		buf, include_headers, write_virtual_data);
}

} //namespace pe_bliss::image
