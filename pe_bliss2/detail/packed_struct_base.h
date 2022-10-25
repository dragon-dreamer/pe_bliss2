#pragma once

#include <boost/endian/conversion.hpp>

#include "pe_bliss2/packed_struct.h"

namespace pe_bliss::detail
{

template<typename Struct,
	boost::endian::order Endianness = boost::endian::order::little>
class packed_struct_base
{
public:
	using underlying_struct_type = Struct;
	using descriptor_type = typename packed_struct<Struct, Endianness>;

public:
	[[nodiscard]]
	descriptor_type& get_descriptor() noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	const descriptor_type& get_descriptor() const noexcept
	{
		return descriptor_;
	}

protected:
	descriptor_type descriptor_;
};

} //namespace pe_bliss::detail
