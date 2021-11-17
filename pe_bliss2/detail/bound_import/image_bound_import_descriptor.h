#pragma once

#include <cstdint>

#include "pe_bliss2/pe_types.h"

namespace pe_bliss::detail::bound_import
{

struct image_bound_import_descriptor
{
	std::uint32_t time_date_stamp;
	std::uint16_t offset_module_name;
	std::uint16_t number_of_module_forwarder_refs;
	//Array of zero or more image_bound_forwarder_ref follows
};

struct image_bound_forwarder_ref
{
	std::uint32_t time_date_stamp;
	std::uint16_t offset_module_name;
	std::uint16_t reserved;
};

} //namespace pe_bliss::detail::bound_import
