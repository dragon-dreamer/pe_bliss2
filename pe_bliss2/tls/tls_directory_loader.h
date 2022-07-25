#pragma once

#include <optional>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/tls/tls_directory.h"

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::tls
{

enum class tls_directory_loader_errc
{
	invalid_raw_data = 1,
	invalid_directory,
	invalid_callbacks,
	invalid_callback_va,
	invalid_index_va
};

std::error_code make_error_code(tls_directory_loader_errc) noexcept;

struct loader_options
{
	bool include_headers = true;
	bool allow_virtual_data = false;
	bool copy_raw_data = false;
};

std::optional<tls_directory_details> load(const image::image& instance,
	const loader_options& options = {});

} //namespace pe_bliss::tls

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::tls::tls_directory_loader_errc> : true_type {};
} //namespace std
