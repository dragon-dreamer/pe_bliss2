#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/tls/tls_directory.h"
#include "pe_bliss2/pe_types.h"

namespace buffers
{
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss
{
class image;
} //namespace pe_bliss

namespace pe_bliss::tls
{

enum class tls_directory_builder_errc
{
	invalid_raw_data_size = 1
};

std::error_code make_error_code(tls_directory_builder_errc) noexcept;

struct builder_options
{
	rva_type directory_rva = 0;
	bool write_virtual_part = false;
	bool update_data_directory = true;
	bool align_index = true;
	bool align_callbacks = true;
};

struct build_result
{
	std::uint32_t full_size{};
	std::uint32_t directory_size{};
};

void build_in_place(image& instance, const tls_directory_details& table,
	const builder_options& options);
void build_in_place(image& instance, const tls_directory& table,
	const builder_options& options);
build_result build_new(image& instance, tls_directory_details& table,
	const builder_options& options);
build_result build_new(image& instance, tls_directory& table,
	const builder_options& options);
build_result build_new(buffers::output_buffer_interface& buf, tls_directory_details& table,
	const builder_options& options, std::uint64_t image_base);
build_result build_new(buffers::output_buffer_interface& buf, tls_directory& table,
	const builder_options& options, std::uint64_t image_base);

[[nodiscard]]
build_result get_built_size(const tls_directory_details& table,
	const builder_options& options) noexcept;
[[nodiscard]]
build_result get_built_size(const tls_directory& table,
	const builder_options& options) noexcept;

} //namespace pe_bliss::tls

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::tls::tls_directory_builder_errc> : true_type {};
} //namespace std
