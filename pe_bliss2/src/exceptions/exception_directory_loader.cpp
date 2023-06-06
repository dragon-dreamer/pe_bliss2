#include "pe_bliss2/exceptions/exception_directory_loader.h"

#include <string>
#include <system_error>

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/exceptions/arm/arm_exception_directory_loader.h"
#include "pe_bliss2/exceptions/arm64/arm64_exception_directory_loader.h"
#include "pe_bliss2/exceptions/x64/x64_exception_directory_loader.h"

namespace
{

struct exception_directory_loader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "exception_directory_loader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::exceptions::exception_directory_loader_errc;
		switch (static_cast<pe_bliss::exceptions::exception_directory_loader_errc>(ev))
		{
		case invalid_x64_exception_directory:
			return "Invalid x64 exception directory";
		case invalid_arm64_exception_directory:
			return "Invalid ARM64 exception directory";
		case invalid_arm_exception_directory:
			return "Invalid ARM exception directory";
		default:
			return {};
		}
	}
};

const exception_directory_loader_error_category exception_directory_loader_error_category_instance;

} //namespace

namespace pe_bliss::exceptions
{

std::error_code make_error_code(exception_directory_loader_errc e) noexcept
{
	return { static_cast<int>(e), exception_directory_loader_error_category_instance };
}

exception_directory_details load(const image::image& instance, const loader_options& options)
{
	exception_directory_details result;

	try
	{
		x64::load(instance, options.x64_loader_options, result);
	}
	catch (const std::system_error&)
	{
		result.add_error(exception_directory_loader_errc::invalid_x64_exception_directory);
	}

	try
	{
		arm64::load(instance, options.arm64_loader_options, result);
	}
	catch (const std::system_error&)
	{
		result.add_error(exception_directory_loader_errc::invalid_arm64_exception_directory);
	}

	try
	{
		arm::load(instance, options.arm_loader_options, result);
	}
	catch (const std::system_error&)
	{
		result.add_error(exception_directory_loader_errc::invalid_arm_exception_directory);
	}

	return result;
}

} //namespace pe_bliss::exceptions
