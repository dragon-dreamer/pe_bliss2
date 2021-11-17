#include "pe_bliss2/image_signature.h"

#include <system_error>

#include "pe_bliss2/pe_error.h"

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
		switch (static_cast<pe_bliss::image_signature_errc>(ev))
		{
		case pe_bliss::image_signature_errc::invalid_pe_signature:
			return "Invalid PE signature";
		default:
			return {};
		}
	}
};

const image_signature_error_category image_signature_error_category_instance;

} //namespace

namespace pe_bliss
{

std::error_code make_error_code(image_signature_errc e) noexcept
{
	return { static_cast<int>(e), image_signature_error_category_instance };
}

pe_error_wrapper image_signature::validate() const noexcept
{
	if (base_struct().get() != pe_signature)
		return image_signature_errc::invalid_pe_signature;
	return {};
}

} //namespace pe_bliss
