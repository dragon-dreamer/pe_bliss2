#include "image_factory.h"

#include <fstream>
#include <memory>

#include "buffers/input_stream_buffer.h"
#include "pe_bliss2/image.h"

pe_bliss::image load_image(const char* filename,
	const pe_bliss::image_load_options& options)
{
	auto pe_file = std::make_shared<std::ifstream>(
		filename, std::ios::in | std::ios::binary);
	pe_file->exceptions(std::ios::badbit);
	return pe_bliss::image_loader::load(
		std::make_shared<buffers::input_stream_buffer>(pe_file), options);
}
