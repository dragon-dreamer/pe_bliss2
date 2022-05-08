#include "pe_bliss2/core/image_signature_errc.h"

#include <string>
#include <system_error>

namespace
{

struct image_signature_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "image_signature";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::core::image_signature_errc;
		switch (static_cast<pe_bliss::core::image_signature_errc>(ev))
		{
		case invalid_pe_signature:
			return "Invalid PE signature";
		case unable_to_read_pe_signature:
			return "Unable to read PE signature";
		default:
			return {};
		}
	}
};

const image_signature_error_category image_signature_error_category_instance;

} //namespace

namespace pe_bliss::core
{
std::error_code make_error_code(image_signature_errc e) noexcept
{
	return { static_cast<int>(e), image_signature_error_category_instance };
}
} //namespace pe_bliss::core
