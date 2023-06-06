#pragma once

#include <cstdint>

#include "pe_bliss2/exports/export_directory.h"
#include "pe_bliss2/pe_types.h"

namespace buffers
{
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::exports
{

struct builder_options
{
	rva_type directory_rva = 0;
	bool write_virtual_part = false;
	bool update_data_directory = true;
};

void build_in_place(image::image& instance, const export_directory_details& directory,
	const builder_options& options);
void build_in_place(image::image& instance, const export_directory& directory,
	const builder_options& options);
std::uint32_t build_new(image::image& instance, export_directory& directory,
	const builder_options& options);
std::uint32_t build_new(image::image& instance, export_directory_details& directory,
	const builder_options& options);
std::uint32_t build_new(buffers::output_buffer_interface& buf, export_directory& directory,
	const builder_options& options);
std::uint32_t build_new(buffers::output_buffer_interface& buf, export_directory_details& directory,
	const builder_options& options);

[[nodiscard]]
std::uint32_t get_built_size(const export_directory& table);
[[nodiscard]]
std::uint32_t get_built_size(const export_directory_details& table);

} //namespace pe_bliss::exports
