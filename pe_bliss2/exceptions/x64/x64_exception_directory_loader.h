#pragma once

#include <system_error>
#include <type_traits>

#include "pe_bliss2/exceptions/exception_directory.h"

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::exceptions::x64
{

enum class exception_directory_loader_errc
{
	unaligned_runtime_function_entry = 1,
	invalid_unwind_info_flags,
	excessive_data_in_directory,
	unaligned_unwind_info,
	invalid_unwind_slot_count,
	unknown_unwind_code,
	push_nonvol_uwop_out_of_order,
	invalid_runtime_function_entry,
	invalid_unwind_info,
	invalid_exception_handler_rva,
	invalid_chained_runtime_function_entry,
	both_set_fpreg_types_used
};

std::error_code make_error_code(exception_directory_loader_errc) noexcept;

struct [[nodiscard]] loader_options
{
	bool include_headers = true;
	bool allow_virtual_data = false;
};

void load(const image::image& instance, const loader_options& options,
	pe_bliss::exceptions::exception_directory_details& directory);

} //namespace pe_bliss::exceptions::x64

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::exceptions::x64::exception_directory_loader_errc> : true_type {};
} //namespace std
