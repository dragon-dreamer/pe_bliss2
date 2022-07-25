#include "gtest/gtest.h"

#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_errc.h"
#include "pe_bliss2/image/section_data_length_from_va.h"
#include "pe_bliss2/pe_types.h"

#include "tests/tests/pe_bliss2/image_helper.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

TEST(SectionDataLengthFromRvaTests, SectionDataLengthFromRvaTest1)
{
	auto instance = create_test_image({});
	instance.update_full_headers_buffer(false);

	expect_throw_pe_error([&instance] {
		(void)section_data_length_from_rva(instance, 1u, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	auto headers_size = instance.get_full_headers_buffer().size();
	EXPECT_EQ(section_data_length_from_rva(instance, 1u, true, false),
		headers_size - 1u);

	EXPECT_EQ(section_data_length_from_rva(instance,
		static_cast<pe_bliss::rva_type>(headers_size),
		true, false), 0u);
}

TEST(SectionDataLengthFromRvaTests, SectionDataLengthFromRvaTest2)
{
	auto instance = create_test_image({});

	EXPECT_EQ(section_data_length_from_rva(instance, 0x1001u, false, false),
		0x1000u - 1u);

	EXPECT_EQ(section_data_length_from_rva(instance, 0x2000u, false, true),
		0x2000u);
	EXPECT_EQ(section_data_length_from_rva(instance, 0x2000u, false, false),
		0x1000u);

	EXPECT_EQ(section_data_length_from_rva(instance, 0x4000u, false, false),
		0u);
	EXPECT_EQ(section_data_length_from_rva(instance, 0x4001u, false, false),
		0u);
	EXPECT_EQ(section_data_length_from_rva(instance, 0x4000u, false, true),
		0x3000u);
}

TEST(SectionDataLengthFromRvaTests, SectionDataLengthFromVaSectionEntTest)
{
	auto instance = create_test_image({});

	static constexpr std::uint32_t last_section_last_rva = 0x7000u;
	EXPECT_EQ(section_data_length_from_rva(instance, last_section_last_rva, false, true),
		0u);
}

namespace
{
template<typename T>
class SectionDataLengthFromRvaTypedTests : public testing::Test
{
public:
	using type = T;
};

using tested_types = ::testing::Types<std::uint32_t, std::uint64_t>;
} //namespace

TYPED_TEST_SUITE(SectionDataLengthFromRvaTypedTests, tested_types);

TYPED_TEST(SectionDataLengthFromRvaTypedTests, SectionDataLengthFromVaTest1)
{
	using va_type = typename TestFixture::type;

	test_image_options options;
	auto instance = create_test_image(options);
	va_type image_base = options.image_base;
	instance.update_full_headers_buffer(false);

	expect_throw_pe_error([&instance, image_base] {
		(void)section_data_length_from_va(instance, image_base + 1u, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	auto headers_size = static_cast<va_type>(
		instance.get_full_headers_buffer().size());
	EXPECT_EQ(section_data_length_from_va(instance, image_base + 1u, true, false),
		headers_size - 1u);

	EXPECT_EQ(section_data_length_from_va(instance, image_base + headers_size,
		true, false), 0u);
}

TYPED_TEST(SectionDataLengthFromRvaTypedTests, SectionDataLengthFromVaTest2)
{
	using va_type = typename TestFixture::type;

	test_image_options options;
	auto instance = create_test_image(options);
	va_type image_base = options.image_base;

	EXPECT_EQ(section_data_length_from_va(instance, image_base + 0x1001u, false, false),
		0x1000u - 1u);

	EXPECT_EQ(section_data_length_from_va(instance, image_base + 0x2000u, false, true),
		0x2000u);
	EXPECT_EQ(section_data_length_from_va(instance, image_base + 0x2000u, false, false),
		0x1000u);

	EXPECT_EQ(section_data_length_from_va(instance, image_base + 0x4000u, false, false),
		0u);
	EXPECT_EQ(section_data_length_from_va(instance, image_base + 0x4001u, false, false),
		0u);
	EXPECT_EQ(section_data_length_from_va(instance, image_base + 0x4000u, false, true),
		0x3000u);
}

TYPED_TEST(SectionDataLengthFromRvaTypedTests, SectionDataLengthFromVaSectionEntTest)
{
	using va_type = typename TestFixture::type;
	test_image_options options;
	auto instance = create_test_image(options);
	va_type image_base = options.image_base;

	static constexpr std::uint32_t last_section_last_rva = 0x7000u;
	EXPECT_EQ(section_data_length_from_va(instance,
		image_base + last_section_last_rva, false, true), 0u);
}

TEST(SectionDataLengthFromRvaTests, SectionDataLengthFromVaTest3)
{
	auto instance = create_test_image({});

	expect_throw_pe_error([&instance] {
		(void)section_data_length_from_va(instance, 1u, false, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);
}
