#include "utilities/generic_error.h"

#include <system_error>

namespace
{

struct generic_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "generic_error";
	}

	std::string message(int ev) const override
	{
		using enum utilities::generic_errc;
		switch (static_cast<utilities::generic_errc>(ev))
		{
		case integer_overflow:
			return "Integer overflow";
		case buffer_overrun:
			return "Buffer overrun";
		default:
			return {};
		}
	}
};

const generic_error_category generic_error_category_instance;

} //namespace

namespace utilities
{

std::error_code make_error_code(generic_errc e) noexcept
{
	return { static_cast<int>(e), generic_error_category_instance };
}

} //namespace utilities
