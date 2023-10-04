#pragma once

#include <cstddef>
#include <exception>

#include "pe_bliss2/pe_error.h"

#include "simple_asn1/der_decode.h"

namespace pe_bliss::security
{

template<auto ErrorValue, typename Spec, typename Range, typename Result>
void decode_asn1_check_tail(Range&& range, Result& result)
{
	try
	{
		auto end = asn1::der::decode<Spec>(range.begin(), range.end(), result);
		while (end != range.end())
		{
			if (*end++ != std::byte{})
				throw pe_error(ErrorValue);
		}
	}
	catch (const asn1::parse_error&)
	{
		std::throw_with_nested(pe_error(ErrorValue));
	}
}

} //namespace pe_bliss::security
