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
	using packed_struct_type = typename packed_struct<Struct, Endianness>;

public:
	[[nodiscard]]
	packed_struct_type& base_struct() noexcept
	{
		return struct_;
	}

	[[nodiscard]]
	const packed_struct_type& base_struct() const noexcept
	{
		return struct_;
	}

private:
	packed_struct_type struct_;
};

} //namespace pe_bliss::detail
