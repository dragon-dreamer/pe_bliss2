#include "pe_bliss2/resources/resource_reader_errc.h"

#include <string>
#include <system_error>

namespace
{

struct resource_reader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "resource_reader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::resources::resource_reader_errc;
		switch (static_cast<pe_bliss::resources::resource_reader_errc>(ev))
		{
		case invalid_buffer_size:
			return "Invalid buffer size";
		case buffer_read_error:
			return "Buffer read error";
		default:
			return {};
		}
	}
};

const resource_reader_error_category resource_reader_error_category_instance;
}

namespace pe_bliss::resources
{

std::error_code make_error_code(resource_reader_errc e) noexcept
{
	return { static_cast<int>(e), resource_reader_error_category_instance };
}

} // namespace pe_bliss::resources
