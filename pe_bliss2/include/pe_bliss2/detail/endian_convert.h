#pragma once

#include <boost/endian/conversion.hpp>

namespace pe_bliss::detail
{

template<boost::endian::order From, boost::endian::order To, typename Val>
void convert_endianness(Val& value) noexcept
{
	if constexpr (From != To)
	{
		boost::endian::conditional_reverse_inplace<From, To>(value);
	}
}

} //namespace pe_bliss::detail
