#pragma once

#include <array>

#include "pe_bliss2/detail/packed_serialization.h"
#include "pe_bliss2/detail/packed_reflection.h"

#include "buffers/output_buffer_interface.h"

namespace pe_bliss::detail
{

template<boost::endian::order StructureFieldsEndianness
	= boost::endian::order::native>
class packed_buffer_serialization
{
public:
	template<standard_layout T>
	static void serialize(const T& value, buffers::output_buffer_interface& buffer)
	{
		std::array<std::byte, packed_reflection::get_type_size<T>()> data{};
		packed_serialization<StructureFieldsEndianness>::serialize(value, data.data());
		buffer.write(data.size(), data.data());
	}
};

} //namespace pe_bliss::detail
