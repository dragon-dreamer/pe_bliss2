#pragma once

#include <system_error>
#include <type_traits>

#include "pe_bliss2/exceptions/exception_directory.h"
#include "pe_bliss2/exceptions/x64/x64_exception_directory_loader.h"
#include "pe_bliss2/exceptions/arm64/arm64_exception_directory_loader.h"
#include "pe_bliss2/exceptions/arm/arm_exception_directory_loader.h"

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::exceptions
{

enum class exception_directory_loader_errc
{
	invalid_x64_exception_directory = 1,
	invalid_arm64_exception_directory,
	invalid_arm_exception_directory
};

std::error_code make_error_code(exception_directory_loader_errc) noexcept;

struct loader_options
{
	x64::loader_options x64_loader_options;
	arm64::loader_options arm64_loader_options;
	arm::loader_options arm_loader_options;
};

exception_directory_details load(const image::image& instance, const loader_options& options);

} //namespace pe_bliss::exceptions

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::exceptions::exception_directory_loader_errc> : true_type {};
} //namespace std
