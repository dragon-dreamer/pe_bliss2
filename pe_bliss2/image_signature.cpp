#include "pe_bliss2/image_signature.h"

#include <exception>
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
		using enum pe_bliss::image_signature_errc;
		switch (static_cast<pe_bliss::image_signature_errc>(ev))
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

namespace pe_bliss
{

std::error_code make_error_code(image_signature_errc e) noexcept
{
	return { static_cast<int>(e), image_signature_error_category_instance };
}

void image_signature::deserialize(buffers::input_buffer_interface& buf,
	bool allow_virtual_memory)
{
	try
	{
		base_struct().deserialize(buf, allow_virtual_memory);
	}
	catch (...)
	{
		std::throw_with_nested(pe_error(
			image_signature_errc::unable_to_read_pe_signature));
	}
}

void image_signature::serialize(buffers::output_buffer_interface& buf,
	bool write_virtual_part) const
{
	base_struct().serialize(buf, write_virtual_part);
}

pe_error_wrapper image_signature::validate() const noexcept
{
	if (base_struct().get() != pe_signature)
		return image_signature_errc::invalid_pe_signature;
	return {};
}

} //namespace pe_bliss
