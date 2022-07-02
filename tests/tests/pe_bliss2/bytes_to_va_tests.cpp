#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <system_error>

#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/image/bytes_to_va.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_errc.h"
#include "pe_bliss2/packed_byte_array.h"
#include "pe_bliss2/packed_byte_vector.h"
#include "pe_bliss2/pe_types.h"

#include "tests/tests/pe_bliss2/bytes_to_va_fixture_base.h"
#include "tests/tests/pe_bliss2/image_helper.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

namespace
{

class BytesToVaFixture : public BytesToVaFixtureBase
{
public:
	static constexpr std::uint32_t buffer_absolute_offset = 0x12u;

	using section_arr_type = decltype(section_arr);
	static constexpr auto virtual_section_arr_size
		= std::tuple_size_v<section_arr_type> + 1u;
	using header_arr_type = decltype(header_arr);
	static constexpr auto virtual_header_arr_size
		= std::tuple_size_v<header_arr_type> +1u;

	template<typename ArrayOrVector>
	std::uint32_t bytes_to_address(pe_bliss::rva_type rva,
		const ArrayOrVector& arr, bool include_headers, bool write_virtual_part)
	{
		auto image_base = instance.get_optional_header().get_raw_image_base();
		switch (GetParam())
		{
		case function_type::va32:
			return static_cast<std::uint32_t>(bytes_to_va(
				instance, static_cast<std::uint32_t>(rva
					+ image_base), arr, include_headers, write_virtual_part)
				- image_base);
		case function_type::va64:
			return static_cast<std::uint32_t>(bytes_to_va(
				instance, static_cast<std::uint64_t>(rva
					+ image_base), arr, include_headers, write_virtual_part)
				- image_base);
		default: //rva
			return bytes_to_rva(instance, rva, arr, include_headers,
				write_virtual_part);
		}
	}

	template<typename Arr, std::size_t Size = std::tuple_size_v<Arr>>
	static pe_bliss::packed_byte_array<Size> create_array(const Arr& arr)
	{
		pe_bliss::packed_byte_array<Size> result;
		auto size = std::min(Size, arr.size());
		std::copy(arr.begin(), arr.begin() + size, result.value().begin());
		result.set_physical_size(size);
		return result;
	}

	template<typename Arr>
	static pe_bliss::packed_byte_vector create_vector(const Arr& arr)
	{
		pe_bliss::packed_byte_vector result;
		result.value().resize(arr.size());
		std::copy(arr.begin(), arr.end(), result.value().begin());
		return result;
	}
};

} //namespace

TEST_P(BytesToVaFixture, ArrayToSectionTest)
{
	EXPECT_EQ(bytes_to_address(section_arr_rva, create_array(section_arr), false, false),
		section_arr_rva + section_arr.size());
	check_section_data_equals();
}

TEST_P(BytesToVaFixture, PhysicalArrayToSectionTest)
{
	auto arr = create_array<section_arr_type, virtual_section_arr_size>(section_arr);
	arr.set_data_size(virtual_section_arr_size);
	EXPECT_EQ(bytes_to_address(section_arr_rva, arr,
		false, false), section_arr_rva + section_arr.size());
	check_section_data_equals();
}

TEST_P(BytesToVaFixture, VirtualArrayToSectionErrorTest)
{
	auto arr = create_array<section_arr_type, virtual_section_arr_size>(section_arr);
	arr.set_data_size(virtual_section_arr_size);
	EXPECT_THROW((void)bytes_to_address(section_arr_rva, arr,
		false, true), std::system_error);
}

TEST_P(BytesToVaFixture, VectorToSectionTest)
{
	EXPECT_EQ(bytes_to_address(section_arr_rva, create_vector(section_arr), false, false),
		section_arr_rva + section_arr.size());
	check_section_data_equals();
}

TEST_P(BytesToVaFixture, PhysicalVectorToSectionTest)
{
	auto arr = create_vector(section_arr);
	arr.set_data_size(virtual_section_arr_size);
	EXPECT_EQ(bytes_to_address(section_arr_rva, arr,
		false, false), section_arr_rva + section_arr.size());
	check_section_data_equals();
}

TEST_P(BytesToVaFixture, VirtualVectorToSectionErrorTest)
{
	auto arr = create_vector(section_arr);
	arr.set_data_size(virtual_section_arr_size);
	EXPECT_THROW((void)bytes_to_address(section_arr_rva, arr,
		false, true), std::system_error);
}

TEST_P(BytesToVaFixture, ArrayToHeaderTest)
{
	EXPECT_EQ(bytes_to_address(header_arr_offset, create_array(header_arr), true, false),
		header_arr_offset + header_arr.size());
	check_header_data_equals();
}

TEST_P(BytesToVaFixture, PhysicalArrayToHeaderTest)
{
	auto arr = create_array<header_arr_type, virtual_header_arr_size>(header_arr);
	arr.set_data_size(virtual_header_arr_size);
	EXPECT_EQ(bytes_to_address(header_arr_offset, arr, true, false),
		header_arr_offset + header_arr.size());
	check_header_data_equals();
}

TEST_P(BytesToVaFixture, VirtualArrayToHeaderErrorTest)
{
	auto arr = create_array<header_arr_type, virtual_header_arr_size>(header_arr);
	arr.set_data_size(virtual_header_arr_size);
	EXPECT_THROW((void)bytes_to_address(header_arr_offset, arr,
		true, true), std::system_error);
}

TEST_P(BytesToVaFixture, VectorToHeaderTest)
{
	EXPECT_EQ(bytes_to_address(header_arr_offset, create_vector(header_arr), true, false),
		header_arr_offset + header_arr.size());
	check_header_data_equals();
}

TEST_P(BytesToVaFixture, PhysicalVectorToHeaderTest)
{
	auto arr = create_vector(header_arr);
	arr.set_data_size(virtual_header_arr_size);
	EXPECT_EQ(bytes_to_address(header_arr_offset, arr, true, false),
		header_arr_offset + header_arr.size());
	check_header_data_equals();
}

TEST_P(BytesToVaFixture, VirtualVectorToHeaderErrorTest)
{
	auto arr = create_vector(header_arr);
	arr.set_data_size(virtual_header_arr_size);
	EXPECT_THROW((void)bytes_to_address(header_arr_offset, arr,
		true, true), std::system_error);
}

TEST_P(BytesToVaFixture, ArrayToHeaderErrorTest)
{
	expect_throw_pe_error([this]
	{
		(void)bytes_to_address(header_arr_offset,
			create_array(header_arr), false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST_P(BytesToVaFixture, VectorToHeaderErrorTest)
{
	expect_throw_pe_error([this]
	{
		(void)bytes_to_address(header_arr_offset,
			create_vector(header_arr), false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST(BytesToVaTest, BytesToVaAddressConversionErrorTest)
{
	auto instance = create_test_image({});
	auto arr = BytesToVaFixture::create_array(BytesToVaFixture::section_arr);

	expect_throw_pe_error([&instance, &arr] {
		(void)bytes_to_va(instance,
			static_cast<std::uint32_t>(1u), arr, false, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);

	expect_throw_pe_error([&instance, &arr] {
		(void)bytes_to_va(instance,
			static_cast<std::uint64_t>(1u), arr, false, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);
}

TEST(BytesToVaTest, BytesToFileOffsetHeaderErrorTest)
{
	auto instance = create_test_image({});
	instance.update_full_headers_buffer();
	auto arr = BytesToVaFixture::create_array(BytesToVaFixture::section_arr);
	expect_throw_pe_error([&instance, &arr]
	{
		(void)bytes_to_file_offset(instance, arr, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST(BytesToVaTest, ArrayToFileOffsetHeaderTest)
{
	auto instance = create_test_image({});
	instance.update_full_headers_buffer();
	auto arr = BytesToVaFixture::create_array(BytesToVaFixture::section_arr);
	arr.get_state().set_absolute_offset(BytesToVaFixture::buffer_absolute_offset);
	EXPECT_EQ(bytes_to_file_offset(instance, arr, true, false),
		BytesToVaFixture::buffer_absolute_offset + BytesToVaFixture::section_arr.size());
}

TEST(BytesToVaTest, VectorToFileOffsetHeaderTest)
{
	auto instance = create_test_image({});
	instance.update_full_headers_buffer();
	auto arr = BytesToVaFixture::create_vector(BytesToVaFixture::section_arr);
	arr.get_state().set_absolute_offset(BytesToVaFixture::buffer_absolute_offset);
	EXPECT_EQ(bytes_to_file_offset(instance, arr, true, false),
		BytesToVaFixture::buffer_absolute_offset + BytesToVaFixture::section_arr.size());
}

INSTANTIATE_TEST_SUITE_P(
	BytesToVaTests,
	BytesToVaFixture,
	::testing::Values(
		function_type::rva,
		function_type::va32,
		function_type::va64
	));
