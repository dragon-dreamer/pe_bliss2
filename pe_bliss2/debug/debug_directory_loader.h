#pragma once

#include <cstdint>
#include <optional>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/debug/debug_directory.h"

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::debug
{

enum class debug_directory_loader_errc
{
	no_rva_and_file_offset = 1,
	invalid_file_offset,
	invalid_debug_directory_size,
	excessive_data_in_directory,
	unable_to_load_entries,
	unable_to_load_raw_data,
	rva_and_file_offset_do_not_match,
	too_many_debug_directories,
	too_big_raw_data
};

std::error_code make_error_code(debug_directory_loader_errc) noexcept;

struct [[nodiscard]] loader_options
{
	bool include_headers = true;
	bool include_overlay = true;
	bool allow_virtual_data = false;
	bool copy_raw_data = false;
	std::uint32_t max_debug_directories = 0xffu;
	std::uint32_t max_raw_data_size = 10'000'000;
};

[[nodiscard]]
std::optional<debug_directory_list_details> load(const image::image& instance,
	const loader_options& options = {});

} //namespace pe_bliss::debug

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::debug::debug_directory_loader_errc> : true_type {};
} //namespace std
