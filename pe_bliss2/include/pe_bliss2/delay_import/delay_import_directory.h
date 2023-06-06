#pragma once

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/imports/import_directory.h"
#include "pe_bliss2/detail/delay_import/image_delay_load_descriptor.h"

namespace pe_bliss::delay_import
{

using delay_import_directory = imports::import_directory_base<imports::imported_library,
	detail::delay_import::image_delayload_descriptor>;
class [[nodiscard]] delay_import_directory_details
	: public imports::import_directory_base<imports::imported_library_details,
	detail::delay_import::image_delayload_descriptor>
	, public error_list
{
};

} //namespace pe_bliss::delay_import
