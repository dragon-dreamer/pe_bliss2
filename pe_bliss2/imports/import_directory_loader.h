#pragma once

#include <optional>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/imports/import_directory.h"

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::imports
{

enum class import_table_loader_errc
{
	invalid_library_name = 1,
	invalid_import_hint,
	invalid_import_name,
	invalid_hint_name_rva,
	lookup_and_address_table_thunks_differ,
	invalid_import_directory,
	zero_iat_and_ilt,
	invalid_import_ordinal,
	zero_iat,
	invalid_imported_library_iat_ilt
};

std::error_code make_error_code(import_table_loader_errc) noexcept;

struct loader_options
{
	bool include_headers = true;
	bool allow_virtual_data = false;
	core::data_directories::directory_type target_directory
		= core::data_directories::directory_type::imports;
};

std::optional<import_directory_details> load(const image::image& instance,
	const loader_options& options);

} //namespace pe_bliss::imports

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::imports::import_table_loader_errc> : true_type {};
} //namespace std
