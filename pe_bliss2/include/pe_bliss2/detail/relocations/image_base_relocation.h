#pragma once

#include <cstdint>

#include "pe_bliss2/pe_types.h"

namespace pe_bliss::detail::relocations
{

struct image_base_relocation
{
	rva_type virtual_address;
	std::uint32_t size_of_block;
};

using type_or_offset_entry = std::uint16_t;

} //namespace pe_bliss::detail::relocations
