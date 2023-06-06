#pragma once

#include <cstdint>

namespace pe_bliss::detail
{

struct image_file_header
{
	std::uint16_t machine;
	std::uint16_t number_of_sections;
	std::uint32_t time_date_stamp;
	std::uint32_t pointer_to_symbol_table;
	std::uint32_t number_of_symbols;
	std::uint16_t size_of_optional_header;
	std::uint16_t characteristics;
};

} //namespace pe_bliss::detail
