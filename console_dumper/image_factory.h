#pragma once

#include "pe_bliss2/image.h"
#include "pe_bliss2/image_loader.h"

pe_bliss::image load_image(const char* filename,
	const pe_bliss::image_load_options& options = {});
