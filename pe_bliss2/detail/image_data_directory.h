#pragma once

#include <cstdint>

namespace pe_bliss::detail
{

struct image_data_directory
{
	std::uint32_t virtual_address;
	std::uint32_t size;
};

} //namespace pe_bliss::detail
