#include "pe_bliss2/packed_byte_vector.h"

#include <cstddef>
#include <vector>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/generic_error.h"

namespace pe_bliss
{

void packed_byte_vector::deserialize(buffers::input_buffer_stateful_wrapper_ref& buf,
	std::size_t size, bool allow_virtual_data)
{
	buffers::serialized_data_state state(buf);

	vector_type value;
	value.resize(size);
	value.resize(buf.read(size, value.data()));

	if (!allow_virtual_data && value.size() != size)
		throw pe_error(utilities::generic_errc::buffer_overrun);

	value_ = std::move(value);
	virtual_size_ = size;
	state_ = state;
}

std::size_t packed_byte_vector::serialize(buffers::output_buffer_interface& buf,
	bool write_virtual_part) const
{
	buf.write(value_.size(), value_.data());

	if (write_virtual_part && is_virtual())
	{
		std::byte zero{};
		for (std::size_t i = value_.size(); i < virtual_size_; ++i)
			buf.write(sizeof(zero), &zero);
	}

	return write_virtual_part ? data_size() : value_.size();
}

std::size_t packed_byte_vector::serialize(std::byte* buf,
	std::size_t max_size, bool write_virtual_part) const
{
	std::size_t size = write_virtual_part ? data_size() : value_.size();
	if (size > max_size)
		throw pe_error(utilities::generic_errc::buffer_overrun);

	std::memcpy(buf, value_.data(), value_.size());
	if (write_virtual_part && is_virtual())
		std::memset(buf + value_.size(), 0, data_size() - value_.size());

	return size;
}

} //namespace pe_bliss
