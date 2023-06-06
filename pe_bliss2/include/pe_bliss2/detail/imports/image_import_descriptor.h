#pragma once

#include <cstdint>

#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/pe_types.h"

namespace pe_bliss::detail::imports
{

template<executable_pointer Pointer>
using image_thunk_data = Pointer;

struct image_import_descriptor
{
	rva_type lookup_table;
	std::uint32_t time_date_stamp;
	std::uint32_t forwarder_chain;
	rva_type name;
	rva_type address_table;
};

constexpr std::uint64_t image_ordinal_flag64 = 0x8000000000000000ull;
constexpr std::uint32_t image_ordinal_flag32 = 0x80000000u;

} //namespace pe_bliss::detail::imports
