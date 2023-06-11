#include "pe_bliss2/packed_c_string.h"

#include <cstddef>
#include <string>

#include <boost/endian/conversion.hpp>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/generic_error.h"

namespace pe_bliss
{

template<typename String>
void packed_c_string_base<String>::deserialize(
	buffers::input_buffer_stateful_wrapper_ref& buf,
	bool allow_virtual_data,
	std::size_t max_physical_size)
{
	buffers::serialized_data_state state(buf);
	
	typename string_type::value_type ch{};
	static constexpr typename string_type::value_type nullbyte{};
	string_type value;
	std::size_t read_bytes{};
	while ((read_bytes = buf.read(sizeof(ch),
		reinterpret_cast<std::byte*>(&ch))) == sizeof(ch))
	{
		if (max_physical_size < sizeof(ch))
			throw pe_error(utilities::generic_errc::buffer_overrun);
		max_physical_size -= sizeof(ch);

		boost::endian::little_to_native_inplace(ch);
		if (ch == nullbyte)
		{
			value_ = std::move(value);
			state_ = state;
			virtual_nullbyte_ = false;
			return;
		}
		value.push_back(ch);
	}

	if (!allow_virtual_data)
		throw pe_error(utilities::generic_errc::buffer_overrun);

	if (max_physical_size < sizeof(ch)) // virtual nullbyte
		throw pe_error(utilities::generic_errc::buffer_overrun);

	if (read_bytes && ch)
	{
		//ch has a non-zero value, this was not a nullbyte
		boost::endian::little_to_native_inplace(ch);
		value.push_back(ch);
		buf.advance_rpos(sizeof(ch));
		max_physical_size -= sizeof(ch);
		if (max_physical_size < sizeof(ch)) // virtual nullbyte
			throw pe_error(utilities::generic_errc::buffer_overrun);
	}

	value_ = std::move(value);
	virtual_nullbyte_ = true;
	state_ = state;
}

template<typename String>
std::size_t packed_c_string_base<String>::serialize(
	buffers::output_buffer_interface& buf,
	bool write_virtual_part) const
{
	bool write_nullbyte = !virtual_nullbyte_ || write_virtual_part;
	std::size_t size = (value_.size() + write_nullbyte)
		* sizeof(typename string_type::value_type);
	buf.write(size, reinterpret_cast<const std::byte*>(value_.data()));
	return size;
}

template<typename String>
std::size_t packed_c_string_base<String>::serialize(std::byte* buf,
	std::size_t max_size, bool write_virtual_part) const
{
	std::size_t size = value_.size() * sizeof(typename string_type::value_type);
	if (!virtual_nullbyte_ || write_virtual_part)
		size += sizeof(typename string_type::value_type);
	if (size > max_size)
		throw pe_error(utilities::generic_errc::buffer_overrun);
	std::memcpy(buf, value_.data(), size);
	return size;
}

template class packed_c_string_base<std::string>;
template class packed_c_string_base<std::u16string>;

} //namespace pe_bliss
