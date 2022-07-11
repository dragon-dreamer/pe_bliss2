#include "image_factory.h"

#include <fstream>
#include <memory>

#include "buffers/input_stream_buffer.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/image/image.h"

pe_bliss::image::image_load_result load_image(const char* filename,
	const pe_bliss::image::image_load_options& options,
	pe_bliss::error_list& errs)
{
	auto pe_file = std::make_shared<std::ifstream>(
		filename, std::ios::in | std::ios::binary);
	pe_file->exceptions(std::ios::badbit);
	return pe_bliss::image::image_loader::load(
		std::make_shared<buffers::input_stream_buffer>(pe_file), options, errs);
}
