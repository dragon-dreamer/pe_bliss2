#include "pe_bliss2/exceptions/arm_common/arm_common_exception_directory_loader.h"

#include <string>
#include <system_error>

namespace
{

struct arm_common_exception_directory_loader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "arm_common_exception_directory_loader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::exceptions::arm_common::exception_directory_loader_errc;
		switch (static_cast<pe_bliss::exceptions::arm_common::exception_directory_loader_errc>(ev))
		{
		case unordered_epilog_scopes:
			return "Unordered epilog scopes";
		case invalid_extended_unwind_info:
			return "Invalid extended unwind info";
		case invalid_runtime_function_entry:
			return "Invalid runtime function entry";
		case invalid_uwop_code:
			return "Invalid unwind opcode";
		case invalid_directory_size:
			return "Invalid exception directory size";
		case invalid_exception_handler:
			return "Invalid exception handler";
		case invalid_exception_handler_rva:
			return "Invalid exception handler RVA";
		case unmatched_directory_size:
			return "Unmatched directory size";
		default:
			return {};
		}
	}
};

const arm_common_exception_directory_loader_error_category
	arm_common_exception_directory_loader_error_category_instance;

} //namespace

namespace pe_bliss::exceptions::arm_common
{

std::error_code make_error_code(exception_directory_loader_errc e) noexcept
{
	return { static_cast<int>(e), arm_common_exception_directory_loader_error_category_instance };
}

} //namespace namespace pe_bliss::exceptions::arm_common
