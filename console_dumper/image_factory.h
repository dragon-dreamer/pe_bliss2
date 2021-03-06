#pragma once

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_loader.h"

pe_bliss::image::image_load_result load_image(const char* filename,
	const pe_bliss::image::image_load_options& options);
