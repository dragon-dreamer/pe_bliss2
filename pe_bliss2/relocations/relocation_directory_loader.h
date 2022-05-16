#pragma once

#include <optional>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/relocations/base_relocation.h"

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::relocations
{

enum class relocation_directory_loader_errc
{
	invalid_relocation_block_size = 1,
	unaligned_relocation_entry,
	invalid_directory_size
};

std::error_code make_error_code(relocation_directory_loader_errc) noexcept;

struct loader_options
{
	bool include_headers = true;
	bool allow_virtual_data = false;
};

struct relocation_directory
{
	base_relocation_details_list relocations;
	error_list errors;
};

std::optional<relocation_directory> load(const image::image& instance,
	const loader_options& options);

} //namespace pe_bliss::relocations

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::relocations::relocation_directory_loader_errc> : true_type {};
} //namespace std
