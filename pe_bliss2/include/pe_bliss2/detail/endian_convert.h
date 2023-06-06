#pragma once

#include <type_traits>

#include <boost/endian/conversion.hpp>

namespace pe_bliss::detail
{

template<boost::endian::order From, boost::endian::order To, typename Val>
void convert_endianness(Val& value) noexcept
{
	if constexpr (From != To)
	{
		if constexpr (std::is_array_v<std::remove_cvref_t<Val>>)
		{
			for (auto& elem : value)
				convert_endianness<From, To>(elem);
		}
		else
		{
			boost::endian::conditional_reverse_inplace<From, To>(value);
		}
	}
}

} //namespace pe_bliss::detail
