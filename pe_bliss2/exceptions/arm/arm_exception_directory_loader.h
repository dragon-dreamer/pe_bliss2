#pragma once

#include <system_error>
#include <type_traits>

#include "pe_bliss2/exceptions/exception_directory.h"

namespace pe_bliss
{
class image;
} //namespace pe_bliss

namespace pe_bliss::exceptions::arm
{

enum class exception_directory_loader_errc
{
	invalid_runtime_function_entry = 1
};

std::error_code make_error_code(exception_directory_loader_errc) noexcept;

struct loader_options
{
	bool include_headers = true;
	bool allow_virtual_data = false;
};

void load(const image& instance, const loader_options& options,
	pe_bliss::exceptions::exception_directory_details& directory);

} //namespace pe_bliss::exceptions::arm

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::exceptions::arm::exception_directory_loader_errc> : true_type {};
} //namespace std
