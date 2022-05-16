#pragma once

#include <optional>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/bound_import/bound_library.h"

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::bound_import
{

enum class bound_import_directory_loader_errc
{
	invalid_library_name = 1,
	name_offset_overlaps_descriptors
};

std::error_code make_error_code(bound_import_directory_loader_errc) noexcept;

struct loader_options
{
	bool include_headers = true;
	bool allow_virtual_data = false;
};

std::optional<bound_library_details_list> load(const image::image& instance,
	const loader_options& options);

} //namespace pe_bliss::bound_import

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::bound_import::bound_import_directory_loader_errc> : true_type {};
} //namespace std
