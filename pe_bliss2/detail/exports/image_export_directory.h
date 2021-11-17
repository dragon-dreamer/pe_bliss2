#pragma once

#include <cstdint>

namespace pe_bliss::detail::exports
{

struct image_export_directory
{
	uint32_t characteristics;
	uint32_t time_date_stamp;
	uint16_t major_version;
	uint16_t minor_version;
	uint32_t name;
	uint32_t base;
	uint32_t number_of_functions;
	uint32_t number_of_names;
	uint32_t address_of_functions;
	uint32_t address_of_names;
	uint32_t address_of_name_ordinals;
};

} //namespace pe_bliss::detail::exports
