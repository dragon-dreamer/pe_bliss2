#include "gtest/gtest.h"

#include <limits>

#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/core/optional_header.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/pe_types.h"

#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;

namespace
{
static constexpr rva_type image_base = 0x123u;
} //namespace

TEST(AddressConverterTests, RvaToVaTest)
{
	address_converter ac(image_base);
	EXPECT_EQ(ac.rva_to_va<std::uint32_t>(0x456u), image_base + 0x456u);
	EXPECT_EQ(ac.rva_to_va<std::uint64_t>(0x456u), image_base + 0x456u);

	expect_throw_pe_error([&] {
		(void)ac.rva_to_va<std::uint32_t>((std::numeric_limits<rva_type>::max)());
	}, address_converter_errc::address_conversion_overflow);
	EXPECT_EQ(ac.rva_to_va<std::uint64_t>((std::numeric_limits<rva_type>::max)()),
		static_cast<std::uint64_t>(image_base)
			+ (std::numeric_limits<std::uint32_t>::max)());

	std::uint32_t va{};
	EXPECT_NO_THROW(ac.rva_to_va(0x456u, va));
	EXPECT_EQ(va, image_base + 0x456u);

	std::uint64_t va64{};
	EXPECT_NO_THROW(ac.rva_to_va(0x456u, va64));
	EXPECT_EQ(va64, image_base + 0x456u);

	expect_throw_pe_error([&] {
		ac.rva_to_va((std::numeric_limits<rva_type>::max)(), va);
	}, address_converter_errc::address_conversion_overflow);

	EXPECT_NO_THROW(ac.rva_to_va((std::numeric_limits<rva_type>::max)(), va64));
	EXPECT_EQ(va64, static_cast<std::uint64_t>(image_base)
		+ (std::numeric_limits<std::uint32_t>::max)());
}

TEST(AddressConverterTests, ImageBaseTest)
{
	EXPECT_EQ(address_converter(image_base).get_image_base(), image_base);

	core::optional_header oh;
	oh.set_raw_image_base(image_base);
	EXPECT_EQ(address_converter(oh).get_image_base(), image_base);

	image::image img;
	img.get_optional_header().set_raw_image_base(image_base);
	EXPECT_EQ(address_converter(img).get_image_base(), image_base);
}

TEST(AddressConverterTests, VaToRvaTest)
{
	address_converter ac(image_base);
	EXPECT_EQ(ac.va_to_rva(0x456u), 0x456u - image_base);
	EXPECT_EQ(ac.va_to_rva(0x456ull), 0x456ull - image_base);
	expect_throw_pe_error([&] {
		(void)ac.va_to_rva(image_base - 1u);
	}, address_converter_errc::address_conversion_overflow);
	expect_throw_pe_error([&] {
		(void)ac.va_to_rva(static_cast<std::uint64_t>(image_base - 1u));
	}, address_converter_errc::address_conversion_overflow);
}

TEST(AddressConverterTests, VaToRvaTest2)
{
	address_converter ac(image_base);
	expect_throw_pe_error([&] {
		(void)ac.va_to_rva(0xff000000'00000000ull);
	}, address_converter_errc::address_conversion_overflow);
}
