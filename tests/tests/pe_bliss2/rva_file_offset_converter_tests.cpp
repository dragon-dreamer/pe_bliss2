#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <system_error>

#include "buffers/input_container_buffer.h"

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/rva_file_offset_converter.h"
#include "pe_bliss2/packed_struct.h"

#include "tests/tests/pe_bliss2/image_helper.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

#include "utilities/generic_error.h"

TEST(RvaFileOffsetConverterTests, FileOffsetToRvaTest)
{
	auto instance = create_test_image({ .start_section_raw_offset = 0x5000u });

	EXPECT_EQ(file_offset_to_rva(instance, 0u), 0u);
	EXPECT_EQ(file_offset_to_rva(instance, 10u), 10u);

	EXPECT_EQ(file_offset_to_rva(instance, 0x5000u), 0x1000u);
	EXPECT_EQ(file_offset_to_rva(instance, 0x5500u), 0x1500u);

	EXPECT_EQ(file_offset_to_rva(instance, 0x6000u), 0x2000u);
	EXPECT_EQ(file_offset_to_rva(instance, 0x6500u), 0x2500u);

	EXPECT_EQ(file_offset_to_rva(instance, 0x6fffu), 0x2fffu);

	expect_throw_pe_error([&instance] {
		(void)file_offset_to_rva(instance, 0x7000u);
	}, utilities::generic_errc::buffer_overrun);

	expect_throw_pe_error([&instance] {
		(void)file_offset_to_rva(instance, 0x1000u);
	}, utilities::generic_errc::buffer_overrun);
}

TEST(RvaFileOffsetConverterTests, RvaToFileOffsetTest)
{
	auto instance = create_test_image({ .start_section_raw_offset = 0x5000u });

	EXPECT_EQ(rva_to_file_offset(instance, 0u), 0u);
	EXPECT_EQ(rva_to_file_offset(instance, 10u), 10u);

	EXPECT_EQ(rva_to_file_offset(instance, 0x1000u), 0x5000u);
	EXPECT_EQ(rva_to_file_offset(instance, 0x1500u), 0x5500u);

	EXPECT_EQ(rva_to_file_offset(instance, 0x2000u), 0x6000u);
	EXPECT_EQ(rva_to_file_offset(instance, 0x2500u), 0x6500u);

	expect_throw_pe_error([&instance] {
		(void)rva_to_file_offset(instance, 0x3000u);
	}, utilities::generic_errc::buffer_overrun);

	expect_throw_pe_error([&instance] {
		(void)rva_to_file_offset(instance, 0x3500u);
	}, utilities::generic_errc::buffer_overrun);

	expect_throw_pe_error([&instance] {
		(void)rva_to_file_offset(instance, 0x100000u);
	}, utilities::generic_errc::buffer_overrun);
}

TEST(RvaFileOffsetConverterTests, AbsoluteOffsetToRvaTest1)
{
	auto instance = create_test_image({ .start_section_raw_offset = 0x5000u });

	pe_bliss::packed_struct<std::uint32_t> obj;
	EXPECT_EQ(absolute_offset_to_rva(instance, obj), 0u);

	obj.get_state().set_absolute_offset(10u);
	EXPECT_EQ(absolute_offset_to_rva(instance, obj), 10u);

	obj.get_state().set_absolute_offset(0x5500u);
	EXPECT_EQ(absolute_offset_to_rva(instance, obj), 0x1500u);

	obj.get_state().set_absolute_offset(0x6500u);
	EXPECT_EQ(absolute_offset_to_rva(instance, obj), 0x2500u);

	obj.get_state().set_absolute_offset(0x7000u);
	expect_throw_pe_error([&instance, &obj] {
		(void)absolute_offset_to_rva(instance, obj);
	}, utilities::generic_errc::buffer_overrun);
}

TEST(RvaFileOffsetConverterTests, AbsoluteOffsetToRvaTest2)
{
	auto instance = create_test_image({ .start_section_raw_offset = 0x5000u });

	buffers::input_container_buffer buf;
	EXPECT_EQ(absolute_offset_to_rva(instance, buf), 0u);

	buf.set_absolute_offset(10u);
	EXPECT_EQ(absolute_offset_to_rva(instance, buf), 10u);

	buf.set_absolute_offset(0x5500u);
	EXPECT_EQ(absolute_offset_to_rva(instance, buf), 0x1500u);

	buf.set_absolute_offset(0x6500u);
	EXPECT_EQ(absolute_offset_to_rva(instance, buf), 0x2500u);

	buf.set_absolute_offset(0x7000u);
	expect_throw_pe_error([&instance, &buf] {
		(void)absolute_offset_to_rva(instance, buf);
	}, utilities::generic_errc::buffer_overrun);

	if constexpr (sizeof(std::size_t) > sizeof(std::uint32_t))
	{
		buf.set_absolute_offset(std::numeric_limits<std::size_t>::max());
		EXPECT_THROW((void)absolute_offset_to_rva(instance, buf), std::system_error);
	}
}
