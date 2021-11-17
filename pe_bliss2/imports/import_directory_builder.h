#pragma once

#include <cstdint>
#include <optional>

#include "pe_bliss2/imports/import_directory.h"
#include "pe_bliss2/pe_types.h"

namespace buffers
{
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss
{
class image;
} //namespace pe_bliss

namespace pe_bliss::imports
{

struct builder_options
{
	rva_type directory_rva = 0;
	std::optional<rva_type> iat_rva;
	bool write_virtual_part = false;
	bool update_import_data_directory = true;
	bool update_delayed_import_data_directory = false;
	bool update_iat_data_directory = true;
};

struct build_result
{
	std::uint32_t full_size{};
	std::uint32_t iat_rva{};
	std::uint32_t iat_size{};
	std::uint32_t descriptors_size{};
};

struct built_size
{
	std::uint32_t directory_size;
	std::uint32_t iat_size;
};

void build_in_place(image& instance, const import_directory_details& directory,
	const builder_options& options);
void build_in_place(image& instance, const import_directory& directory,
	const builder_options& options);
build_result build_new(image& instance, import_directory_details& directory,
	const builder_options& options);
build_result build_new(image& instance, import_directory& directory,
	const builder_options& options);
build_result build_new(buffers::output_buffer_interface& directory_buf,
	buffers::output_buffer_interface* iat_buf, import_directory_details& directory,
	const builder_options& options);
build_result build_new(buffers::output_buffer_interface& directory_buf,
	buffers::output_buffer_interface* iat_buf, import_directory& directory,
	const builder_options& options);

[[nodiscard]]
built_size get_built_size(const import_directory_details& directory, const builder_options& options);
[[nodiscard]]
built_size get_built_size(const import_directory& directory, const builder_options& options);

} //namespace pe_bliss::imports
