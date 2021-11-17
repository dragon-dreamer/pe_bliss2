#pragma once

#include <cstdint>

#include "pe_bliss2/bound_import/bound_library.h"
#include "pe_bliss2/pe_types.h"

namespace buffers
{
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss
{
class image;
} //namespace pe_bliss

namespace pe_bliss::bound_import
{

struct builder_options
{
	rva_type directory_rva = 0;
	bool write_virtual_part = false;
	bool update_data_directory = true;
};

void build_in_place(image& instance, const bound_library_list& directory,
	const builder_options& options);
void build_in_place(image& instance, const bound_library_details_list& directory,
	const builder_options& options);
std::uint32_t build_new(image& instance, bound_library_list& directory,
	const builder_options& options);
std::uint32_t build_new(image& instance, bound_library_details_list& directory,
	const builder_options& options);
std::uint32_t build_new(buffers::output_buffer_interface& buf, bound_library_list& directory);
std::uint32_t build_new(buffers::output_buffer_interface& buf, bound_library_details_list& directory);

[[nodiscard]]
std::uint32_t get_built_size(const bound_library_list& directory) noexcept;
[[nodiscard]]
std::uint32_t get_built_size(const bound_library_details_list& directory) noexcept;

} //namespace pe_bliss::bound_import
