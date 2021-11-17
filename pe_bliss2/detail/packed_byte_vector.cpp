#include "pe_bliss2/detail/packed_byte_vector.h"

#include <cstddef>

#include "buffers/input_buffer_interface.h"
#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/generic_error.h"

namespace pe_bliss::detail
{

void packed_byte_vector::deserialize(buffers::input_buffer_interface& buf,
	std::size_t size, bool allow_virtual_memory)
{
	virtual_size_ = size;
	value_.clear();
	buffer_pos_ = buf.rpos();
	absolute_offset_ = buffer_pos_ + buf.absolute_offset();
	relative_offset_ = buffer_pos_ + buf.relative_offset();

	value_.resize(size);
	value_.resize(buf.read(size, value_.data()));

	if (!allow_virtual_memory && value_.size() != size)
		throw pe_error(utilities::generic_errc::buffer_overrun);
}

std::size_t packed_byte_vector::serialize(buffers::output_buffer_interface& buf,
	bool write_virtual_part) const
{
	buf.write(value_.size(), value_.data());

	if (write_virtual_part && is_virtual())
	{
		std::byte zero{};
		for (std::size_t i = value_.size(); i < virtual_size_; ++i)
			buf.write(1u, &zero);
	}

	return write_virtual_part ? virtual_size_ : value_.size();
}

std::size_t packed_byte_vector::serialize(std::byte* buf,
	std::size_t max_size, bool write_virtual_part) const
{
	std::size_t size = write_virtual_part ? virtual_size_ : value_.size();
	if (size > max_size)
		throw pe_error(utilities::generic_errc::buffer_overrun);

	std::memcpy(buf, value_.data(), value_.size());
	if (write_virtual_part && is_virtual())
		std::memset(buf + value_.size(), 0, virtual_size_ - value_.size());

	return size;
}

} //namespace pe_bliss::detail
