#include "pe_bliss2/packed_utf16_string.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <utility>

#include <boost/endian/conversion.hpp>

#include "buffers/input_buffer_interface.h"
#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/generic_error.h"

namespace pe_bliss
{

static_assert(sizeof(packed_utf16_string::string_type::value_type)
	== sizeof(std::uint16_t),
	"This facility only supports strings with size of a character equal to 2 bytes");

void packed_utf16_string::deserialize(buffers::input_buffer_interface& buf,
	bool allow_virtual_memory)
{
	buffers::serialized_data_state state(buf);

	std::uint16_t string_length{};
	auto size_bytes_read = buf.read(sizeof(string_length),
		reinterpret_cast<std::byte*>(&string_length));
	if (!allow_virtual_memory && size_bytes_read != sizeof(string_length))
		throw pe_error(utilities::generic_errc::buffer_overrun);

	boost::endian::little_to_native_inplace(string_length);
	auto virtual_size = sizeof(string_length)
		+ string_length * sizeof(string_type::value_type);
	auto physical_size = size_bytes_read;

	string_type::value_type ch{};
	std::size_t index = 0;
	string_type value(string_length, u'\0');
	while (string_length && (size_bytes_read = buf.read(sizeof(ch),
		reinterpret_cast<std::byte*>(&ch))) != 0)
	{
		boost::endian::little_to_native_inplace(ch);
		value[index++] = ch;
		physical_size += size_bytes_read;
		--string_length;
		ch = {};
	}
	value.resize(index);

	if (!allow_virtual_memory && physical_size != virtual_size)
		throw pe_error(utilities::generic_errc::buffer_overrun);

	value_ = std::move(value);
	state_ = state;
	physical_size_ = physical_size;
	virtual_size_ = virtual_size;
}

template<typename WriteChar, typename WriteRemaining>
std::size_t packed_utf16_string::serialize(WriteChar&& write_part,
	WriteRemaining&& write_remaining, bool write_virtual_part) const
{
	auto remaining_size = physical_size_;
	auto size = virtual_string_length();

	if (size > (std::numeric_limits<std::uint16_t>::max)())
		throw pe_error(utilities::generic_errc::buffer_overrun);

	auto size16 = static_cast<std::uint16_t>(size);
	boost::endian::native_to_little_inplace(size);
	auto bytes_to_write = (std::min)(remaining_size, sizeof(size16));
	write_part(bytes_to_write, reinterpret_cast<const std::byte*>(&size16));
	remaining_size -= bytes_to_write;

	for (auto ch : value_)
	{
		if (remaining_size >= sizeof(ch))
			bytes_to_write = sizeof(ch);
		else if (remaining_size == sizeof(std::uint8_t))
			bytes_to_write = sizeof(std::uint8_t);
		else
			break;

		boost::endian::native_to_little_inplace(ch);
		write_part(bytes_to_write, reinterpret_cast<const std::byte*>(&ch));
		remaining_size -= bytes_to_write;
	}

	if (write_virtual_part && is_virtual())
		write_remaining(virtual_size_ - physical_size_);

	return write_virtual_part ? virtual_size_ : physical_size_;
}

std::size_t packed_utf16_string::serialize(buffers::output_buffer_interface& buf,
	bool write_virtual_part) const
{
	return serialize([&buf] (std::size_t bytes_to_write, const std::byte* data) {
		buf.write(bytes_to_write, data);
	}, [&buf] (std::size_t remaining) {
		std::byte zero{};
		while (remaining--)
			buf.write(sizeof(zero), &zero);
	}, write_virtual_part);
}

std::size_t packed_utf16_string::serialize(std::byte* buf,
	std::size_t max_size, bool write_virtual_part) const
{
	std::size_t total_size = write_virtual_part
		? data_size() : physical_size_;
	if (total_size > max_size)
		throw pe_error(utilities::generic_errc::buffer_overrun);

	return serialize([&buf](std::size_t bytes_to_write, const std::byte* data) {
		std::memcpy(buf, data, bytes_to_write);
		buf += bytes_to_write;
	}, [&buf](std::size_t remaining) {
		std::memset(buf, 0, remaining);
	}, write_virtual_part);
}

void packed_utf16_string::set_data_size(std::size_t size) noexcept
{
	virtual_size_ = size;
	if (virtual_size_ < physical_size_)
		virtual_size_ = physical_size_;
}

void packed_utf16_string::set_physical_size(std::size_t size) noexcept
{
	physical_size_ = (std::min)(size, sizeof(std::uint16_t)
		+ value_.size() * sizeof(string_type::value_type));
	if (virtual_size_ < physical_size_)
		virtual_size_ = physical_size_;
}

void packed_utf16_string::sync_physical_size() noexcept
{
	set_physical_size((std::numeric_limits<std::size_t>::max)());
}

std::size_t packed_utf16_string::virtual_string_length() const noexcept
{
	return (virtual_size_ - sizeof(std::uint16_t))
		/ sizeof(string_type::value_type);
}

} //namespace pe_bliss
