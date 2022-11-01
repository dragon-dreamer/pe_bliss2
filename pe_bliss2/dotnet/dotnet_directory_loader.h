#pragma once

#include <optional>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/dotnet/dotnet_directory.h"

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::dotnet
{

enum class dotnet_directory_loader_errc
{
	invalid_directory = 1,
	descriptor_and_directory_sizes_do_not_match,
	unsupported_descriptor_size,
	empty_metadata,
	unable_to_load_metadata,
	unable_to_load_resources,
	unable_to_load_strong_name_signature,
	virtual_metadata,
	virtual_resources,
	virtual_strong_name_signature
};

std::error_code make_error_code(dotnet_directory_loader_errc) noexcept;

struct [[nodiscard]] loader_options
{
	bool include_headers = true;
	bool allow_virtual_data = false;
	bool copy_metadata_memory = false;
	bool copy_resource_memory = false;
	bool copy_strong_name_signature_memory = false;
};

[[nodiscard]]
std::optional<cor20_header_details> load(const image::image& instance,
	const loader_options& options = {});

} //namespace pe_bliss::dotnet

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::dotnet::dotnet_directory_loader_errc> : true_type {};
} //namespace std
