#include "pe_bliss2/resources/bitmap_writer.h"

#include "buffers/buffer_copy.h"
#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/resources/bitmap.h"

namespace pe_bliss::resources
{

void write_bitmap(const bitmap& bmp,
	buffers::output_buffer_interface& output,
	const bitmap_write_options& options)
{
	if (options.mode == bitmap_serialization_mode::file)
		bmp.get_file_header().serialize(output, options.write_virtual_part);

	bmp.get_info_header().serialize(output, options.write_virtual_part);
	buffers::copy(*bmp.get_buffer().data(), output, options.write_virtual_part
		? bmp.get_buffer().size() : bmp.get_buffer().physical_size());
}

} //namespace pe_bliss::resources
