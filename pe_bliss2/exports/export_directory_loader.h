#pragma once

#include <cstdint>
#include <optional>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/exports/export_directory.h"

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::exports
{

enum class export_directory_loader_errc
{
	invalid_library_name = 1,
	invalid_forwarded_name,
	invalid_name_list,
	invalid_name_ordinal,
	invalid_name_rva,
	empty_name,
	unsorted_names,
	invalid_rva
};

std::error_code make_error_code(export_directory_loader_errc) noexcept;

struct loader_options
{
	bool include_headers = true;
	bool allow_virtual_data = false;
	std::uint16_t max_number_of_functions = 0xffffu;
	std::uint16_t max_number_of_names = 0xffffu;
};

[[nodiscard]]
std::optional<export_directory_details> load(const image::image& instance,
	const loader_options& options);

} //namespace pe_bliss::exports

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::exports::export_directory_loader_errc> : true_type {};
} //namespace std
