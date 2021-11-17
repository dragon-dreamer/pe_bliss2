#include "pe_bliss2/detail/packed_byte_array.h"

#include <cstring>

#include "buffers/input_buffer_interface.h"
#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/generic_error.h"

namespace pe_bliss::detail
{

void packed_byte_array_base::deserialize_impl(buffers::input_buffer_interface& buf, std::byte* data,
	std::size_t size, bool allow_virtual_memory)
{
	data_size_ = size;
	buffer_pos_ = buf.rpos();
	absolute_offset_ = buffer_pos_ + buf.absolute_offset();
	relative_offset_ = buffer_pos_ + buf.relative_offset();
	physical_size_ = buf.read(size, data);
	if (!allow_virtual_memory && physical_size_ != size)
		throw pe_error(utilities::generic_errc::buffer_overrun);
}

std::size_t packed_byte_array_base::serialize_impl(buffers::output_buffer_interface& buf, const std::byte* data,
	bool write_virtual_part) const
{
	auto size = write_virtual_part ? physical_size_ : data_size_;
	buf.write(size, data);
	return size;
}

std::size_t packed_byte_array_base::serialize_impl(std::byte* buf, std::size_t max_size,
	const std::byte* data, bool write_virtual_part) const
{
	auto size = write_virtual_part ? physical_size_ : data_size_;
	if (size > max_size)
		throw pe_error(utilities::generic_errc::buffer_overrun);
	std::memcpy(buf, data, size);
	return size;
}

} //namespace pe_bliss::detail
