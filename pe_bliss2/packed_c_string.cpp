#include "pe_bliss2/packed_c_string.h"

#include <cstddef>
#include <string>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/generic_error.h"

namespace pe_bliss
{

void packed_c_string::deserialize(buffers::input_buffer_stateful_wrapper_ref& buf,
	bool allow_virtual_memory)
{
	buffers::serialized_data_state state(buf);

	std::byte ch{};
	static constexpr std::byte nullbyte{};
	std::string value;
	while (buf.read(sizeof(ch), &ch))
	{
		if (ch == nullbyte)
		{
			value_ = std::move(value);
			state_ = state;
			virtual_nullbyte_ = false;
			return;
		}
		value.push_back(std::to_integer<char>(ch));
	}

	if (!allow_virtual_memory)
		throw pe_error(utilities::generic_errc::buffer_overrun);

	value_ = std::move(value);
	virtual_nullbyte_ = true;
	state_ = state;
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

} //namespace pe_bliss
