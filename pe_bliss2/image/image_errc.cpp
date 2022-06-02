#include "pe_bliss2/image/image_errc.h"

#include <string>
#include <system_error>

namespace
{

struct image_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "image";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::image::image_errc;
		switch (static_cast<pe_bliss::image::image_errc>(ev))
		{
		case too_many_sections:
			return "Too many sections";
		case too_many_rva_and_sizes:
			return "Too many data directories";
		case section_data_does_not_exist:
			return "Requested section data do not exist";
		default:
			return {};
		}
	}
};

const image_error_category image_error_category_instance;

} //namespace

namespace pe_bliss::image
{

std::error_code make_error_code(image_errc e) noexcept
{
	return { static_cast<int>(e), image_error_category_instance };
}

} //namespace pe_bliss::image
