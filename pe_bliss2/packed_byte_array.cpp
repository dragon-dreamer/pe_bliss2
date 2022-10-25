#include "pe_bliss2/packed_byte_array.h"

#include <cstring>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/generic_error.h"

namespace pe_bliss::detail
{

void packed_byte_array_base::deserialize_impl(
	buffers::input_buffer_stateful_wrapper_ref& buf, std::byte* data,
	std::size_t size, std::size_t max_size, bool allow_virtual_data)
{
	if (size > max_size)
		throw pe_error(utilities::generic_errc::buffer_overrun);

	buffers::serialized_data_state state(buf);
	auto physical_size = buf.read(size, data);
	if (!allow_virtual_data && physical_size != size)
		throw pe_error(utilities::generic_errc::buffer_overrun);

	physical_size_ = physical_size;
	data_size_ = size;
	state_ = state;
}

std::size_t packed_byte_array_base::serialize_impl(
	buffers::output_buffer_interface& buf, const std::byte* data,
	bool write_virtual_part) const
{
	auto size = write_virtual_part ? data_size_ : physical_size_;
	buf.write(size, data);
	return size;
}

std::size_t packed_byte_array_base::serialize_impl(
	std::byte* buf, std::size_t max_size,
	const std::byte* data, bool write_virtual_part) const
{
	auto size = write_virtual_part ? data_size_ : physical_size_;
	if (size > max_size)
		throw pe_error(utilities::generic_errc::buffer_overrun);
	std::memcpy(buf, data, size);
	return size;
}

} //namespace pe_bliss::detail
