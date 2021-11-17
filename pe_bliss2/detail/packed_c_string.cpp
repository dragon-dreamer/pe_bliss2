#include "pe_bliss2/detail/packed_c_string.h"

#include <cstddef>

#include "buffers/input_buffer_interface.h"
#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/generic_error.h"

namespace pe_bliss::detail
{

void packed_c_string::deserialize(buffers::input_buffer_interface& buf,
	bool allow_virtual_memory)
{
	virtual_nullbyte_ = false;
	value_.clear();
	buffer_pos_ = buf.rpos();
	absolute_offset_ = buffer_pos_ + buf.absolute_offset();
	relative_offset_ = buffer_pos_ + buf.relative_offset();

	std::byte ch{};
	std::byte nullbyte{};
	while (buf.read(1, &ch))
	{
		if (ch == nullbyte)
			return;
		value_.push_back(std::to_integer<char>(ch));
	}

	virtual_nullbyte_ = true;
	if (!allow_virtual_memory)
		throw pe_error(utilities::generic_errc::buffer_overrun);
}

std::size_t packed_c_string::serialize(buffers::output_buffer_interface& buf,
	bool write_virtual_part) const
{
	bool write_nullbyte = !virtual_nullbyte_ || write_virtual_part;
	std::size_t size = value_.size() + write_nullbyte;
	buf.write(size, reinterpret_cast<const std::byte*>(value_.data()));
	return size;
}

std::size_t packed_c_string::serialize(std::byte* buf,
	std::size_t max_size, bool write_virtual_part) const
{
	auto size = value_.size();
	if (!virtual_nullbyte_ || write_virtual_part)
		++size;
	if (size > max_size)
		throw pe_error(utilities::generic_errc::buffer_overrun);
	std::memcpy(buf, value_.data(), size);
	return size;
}

} //namespace pe_bliss::detail
