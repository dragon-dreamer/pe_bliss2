#pragma once

#include <optional>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/resources/resource_directory.h"

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::resources
{

enum class resource_directory_loader_errc
{
	invalid_directory_size = 1,
	invalid_resource_directory,
	invalid_resource_directory_number_of_entries,
	invalid_resource_directory_entry,
	invalid_resource_directory_entry_name,
	invalid_number_of_named_and_id_entries,
	invalid_resource_data_entry,
	invalid_resource_data_entry_raw_data
};

struct loader_options
{
	bool include_headers = true;
	bool allow_virtual_data = false;
	bool copy_raw_data = false;
};

std::error_code make_error_code(resource_directory_loader_errc) noexcept;

std::optional<resource_directory_details> load(const image::image& instance,
	const loader_options& options);

} //namespace pe_bliss::resources

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::resources::resource_directory_loader_errc> : true_type {};
} //namespace std
