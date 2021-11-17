#include "pe_bliss2/address_converter.h"

#include <cstdint>
#include <limits>
#include <system_error>

#include "pe_bliss2/image.h"
#include "pe_bliss2/optional_header.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/math.h"
#include "utilities/safe_uint.h"

namespace
{

struct address_converter_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "address_converter";
	}

	std::string message(int ev) const override
	{
		switch (static_cast<pe_bliss::address_converter_errc>(ev))
		{
		case pe_bliss::address_converter_errc::address_conversion_overflow:
			return "Address conversion overflows";
		default:
			return {};
		}
	}
};

const address_converter_error_category address_converter_error_category_instance;

using namespace pe_bliss;

template<typename Va>
void rva_to_va_impl(rva_type rva, std::uint64_t image_base, Va& va)
{
	try
	{
		va = (utilities::safe_uint<Va>(rva) + image_base).value();
	}
	catch (const std::runtime_error&)
	{
		throw pe_error(address_converter_errc::address_conversion_overflow);
	}
}

template<typename Va>
rva_type va_to_rva_impl(Va va, std::uint64_t image_base)
{
	if (va < image_base)
		throw pe_error(address_converter_errc::address_conversion_overflow);

	return static_cast<rva_type>(va - image_base);
}

} //namespace

namespace pe_bliss
{

std::error_code make_error_code(address_converter_errc e) noexcept
{
	return { static_cast<int>(e), address_converter_error_category_instance };
}

address_converter::address_converter(const image& instance) noexcept
	: address_converter(instance.get_optional_header())
{
}

address_converter::address_converter(const optional_header& header) noexcept
	: address_converter(header.get_raw_image_base())
{
}

address_converter::address_converter(std::uint64_t image_base) noexcept
	: image_base_(image_base)
{
}

void address_converter::rva_to_va(rva_type rva, std::uint32_t& va) const
{
	rva_to_va_impl(rva, image_base_, va);
}

void address_converter::rva_to_va(rva_type rva, std::uint64_t& va) const
{
	rva_to_va_impl(rva, image_base_, va);
}

rva_type address_converter::va_to_rva(std::uint32_t va) const
{
	return va_to_rva_impl(va, image_base_);
}

rva_type address_converter::va_to_rva(std::uint64_t va) const
{
	return va_to_rva_impl(va, image_base_);
}

} //namespace pe_bliss
