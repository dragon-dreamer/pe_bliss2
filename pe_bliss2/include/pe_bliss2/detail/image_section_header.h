#pragma once

#include <array>
#include <cstdint>

namespace pe_bliss::detail
{

struct image_section_header
{
	std::array<std::uint8_t, 8u> name;
	std::uint32_t virtual_size;
	std::uint32_t virtual_address;
	std::uint32_t size_of_raw_data;
	std::uint32_t pointer_to_raw_data;
	std::uint32_t pointer_to_relocations;
	std::uint32_t pointer_to_line_numbers;
	std::uint16_t number_of_relocations;
	std::uint16_t number_of_line_numbers;
	std::uint32_t characteristics;
};

} //namespace pe_bliss::detail
