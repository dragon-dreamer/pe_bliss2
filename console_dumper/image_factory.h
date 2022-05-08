#pragma once

#include <optional>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/image.h"
#include "pe_bliss2/image_loader.h"

std::optional<pe_bliss::image> load_image(const char* filename,
	const pe_bliss::image_load_options& options,
	pe_bliss::error_list& errs);
