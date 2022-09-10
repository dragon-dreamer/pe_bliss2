#pragma once

namespace buffers
{
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::resources
{

class bitmap;

enum class bitmap_serialization_mode
{
	file, //write both file and info header
	resource //write info header only
};

struct [[nodiscard]] bitmap_write_options
{
	bitmap_serialization_mode mode = bitmap_serialization_mode::file;
	bool write_virtual_part = false;
};

void write_bitmap(const bitmap& bmp,
	buffers::output_buffer_interface& output,
	const bitmap_write_options& options);

} //namespace pe_bliss::resources
