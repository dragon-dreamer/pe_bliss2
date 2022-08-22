#pragma once

#include "pe_bliss2/exceptions/exception_directory.h"

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::exceptions::arm64
{

struct [[nodiscard]] loader_options
{
	bool include_headers = true;
	bool allow_virtual_data = false;
	bool load_hybrid_pe_directory = true;
};

void load(const image::image& instance, const loader_options& options,
	pe_bliss::exceptions::exception_directory_details& directory);

} //namespace pe_bliss::exceptions::arm64
