#pragma once

#include <optional>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/imports/import_directory_loader.h"
#include "pe_bliss2/delay_import/delay_import_directory.h"

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::delay_import
{

[[nodiscard]]
std::optional<delay_import_directory_details> load(const image::image& instance,
	const imports::loader_options& options = {});

} //namespace pe_bliss::delay_import
