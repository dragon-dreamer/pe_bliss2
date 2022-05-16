#pragma once

#include <cstdint>

#include "pe_bliss2/relocations/base_relocation.h"
#include "pe_bliss2/pe_types.h"

namespace buffers
{
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::relocations
{

struct builder_options
{
	rva_type directory_rva = 0;
	bool write_virtual_part = false;
	bool update_data_directory = true;
	bool align_base_relocation_structures = true;
};

void build_in_place(image::image& instance, const base_relocation_details_list& directory,
	const builder_options& options);
void build_in_place(image::image& instance, const base_relocation_list& directory,
	const builder_options& options);
std::uint32_t build_new(image::image& instance, base_relocation_details_list& directory,
	const builder_options& options);
std::uint32_t build_new(image::image& instance, base_relocation_list& directory,
	const builder_options& options);
std::uint32_t build_new(buffers::output_buffer_interface& buf, base_relocation_details_list& directory,
	const builder_options& options);
std::uint32_t build_new(buffers::output_buffer_interface& buf, base_relocation_list& directory,
	const builder_options& options);

[[nodiscard]]
std::uint32_t get_built_size(const base_relocation_details_list& directory,
	const builder_options& options) noexcept;
[[nodiscard]]
std::uint32_t get_built_size(const base_relocation_list& directory,
	const builder_options& options) noexcept;

} //namespace pe_bliss::relocations
