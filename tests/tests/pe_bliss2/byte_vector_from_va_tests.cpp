#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>

#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/image/byte_vector_from_va.h"
#include "pe_bliss2/packed_byte_vector.h"

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_errc.h"
#include "pe_bliss2/pe_types.h"

#include "tests/pe_bliss2/byte_container_fixture_base.h"
#include "tests/pe_bliss2/image_helper.h"
#include "tests/pe_bliss2/pe_error_helper.h"

#include "utilities/generic_error.h"

namespace
{

class ByteVectorFromVaFixture : public ByteContainerFromVaFixture
{
public:
	decltype(auto) byte_vector_from_address(pe_bliss::rva_type rva,
		std::size_t size, bool include_headers, bool allow_virtual_data)
	{
		switch (GetParam())
		{
		case function_type::va32:
			return byte_vector_from_va(instance, static_cast<std::uint32_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				static_cast<std::uint32_t>(size), include_headers, allow_virtual_data);
		case function_type::va64:
			return byte_vector_from_va(instance, static_cast<std::uint64_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				static_cast<std::uint32_t>(size), include_headers, allow_virtual_data);
		default: //rva
			return byte_vector_from_rva(instance, rva,
				static_cast<std::uint32_t>(size), include_headers, allow_virtual_data);
		}
	}

	void byte_vector_from_address(pe_bliss::rva_type rva,
		std::size_t size, pe_bliss::packed_byte_vector& arr,
		bool include_headers, bool allow_virtual_data)
	{
		switch (GetParam())
		{
		case function_type::va32:
			byte_vector_from_va(instance, static_cast<std::uint32_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				static_cast<std::uint32_t>(size), arr, include_headers, allow_virtual_data);
			break;
		case function_type::va64:
			byte_vector_from_va(instance, static_cast<std::uint64_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				static_cast<std::uint32_t>(size), arr, include_headers, allow_virtual_data);
			break;
		default: //rva
			byte_vector_from_rva(instance, rva,
				static_cast<std::uint32_t>(size), arr, include_headers, allow_virtual_data);
			break;
		}
	}
};

} //namespace

TEST_P(ByteVectorFromVaFixture, PackedByteVectorSectionTest)
{
	auto arr1 = byte_vector_from_address(
		section_arr_rva, section_arr.size(), false, false);
	pe_bliss::packed_byte_vector arr2;
	byte_vector_from_address(
		section_arr_rva, section_arr.size(), arr2, false, false);

	for (const auto& arr : { arr1, arr2 })
	{
		EXPECT_EQ(arr.value(), std::vector(section_arr.begin(), section_arr.end()));
		EXPECT_EQ(arr.get_state().relative_offset(), section_arr_offset);
		EXPECT_EQ(arr.get_state().absolute_offset(), section_arr_offset);
		EXPECT_EQ(arr.get_state().buffer_pos(), 0u);
		EXPECT_EQ(arr.physical_size(), section_arr.size());
		EXPECT_EQ(arr.data_size(), section_arr.size());
		EXPECT_FALSE(arr.is_virtual());
	}
}

TEST_P(ByteVectorFromVaFixture, PackedByteVectorHeaderErrorTest)
{
	EXPECT_THROW((void)byte_vector_from_address(
		header_arr_offset, header_arr.size(), false, false), std::system_error);
	pe_bliss::packed_byte_vector arr;
	EXPECT_THROW((void)byte_vector_from_address(
		header_arr_offset, header_arr.size(), arr, false, false), std::system_error);
}

TEST_P(ByteVectorFromVaFixture, PackedByteVectorHeaderTest)
{
	auto arr1 = byte_vector_from_address(
		header_arr_offset, header_arr.size(), true, false);
	pe_bliss::packed_byte_vector arr2;
	byte_vector_from_address(
		header_arr_offset, header_arr.size(), arr2, true, false);

	for (const auto& arr : { arr1, arr2 })
	{
		EXPECT_EQ(arr.value(), std::vector(header_arr.begin(), header_arr.end()));
		EXPECT_EQ(arr.get_state().relative_offset(), header_arr_offset);
		EXPECT_EQ(arr.get_state().absolute_offset(), header_arr_offset);
		EXPECT_EQ(arr.get_state().buffer_pos(), 0u);
		EXPECT_EQ(arr.physical_size(), header_arr.size());
		EXPECT_EQ(arr.data_size(), header_arr.size());
		EXPECT_FALSE(arr.is_virtual());
	}
}

TEST_P(ByteVectorFromVaFixture, PackedByteVectorCutErrorTest)
{
	EXPECT_THROW((void)byte_vector_from_address(
		section_arr_rva, section_arr.size() + 1u, false, false), std::system_error);
	pe_bliss::packed_byte_vector arr;
	EXPECT_THROW((void)byte_vector_from_address(
		section_arr_rva, section_arr.size() + 1u, arr, false, false), std::system_error);
}

TEST_P(ByteVectorFromVaFixture, PackedByteVectorSectionVirtualDataTest)
{
	auto arr1 = byte_vector_from_address(
		section_arr_rva, section_arr.size() + 1u, false, true);
	pe_bliss::packed_byte_vector arr2;
	byte_vector_from_address(
		section_arr_rva, section_arr.size() + 1u, arr2, false, true);

	for (const auto& arr : { arr1, arr2 })
	{
		EXPECT_EQ(arr.value(), std::vector(section_arr.begin(), section_arr.end()));
		EXPECT_EQ(arr.get_state().relative_offset(), section_arr_offset);
		EXPECT_EQ(arr.get_state().absolute_offset(), section_arr_offset);
		EXPECT_EQ(arr.get_state().buffer_pos(), 0u);
		EXPECT_EQ(arr.physical_size(), section_arr.size());
		EXPECT_EQ(arr.data_size(), section_arr.size() + 1u);
		EXPECT_TRUE(arr.is_virtual());
	}
}

TEST(ByteVectorFromVa, PackedByteVectorFromVaErrorTest)
{
	auto instance = create_test_image({});

	expect_throw_pe_error([&instance] {
		(void)byte_vector_from_va(instance,
			static_cast<std::uint32_t>(1u), 2u, false, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);

	expect_throw_pe_error([&instance] {
		pe_bliss::packed_byte_vector arr;
		(void)byte_vector_from_va(instance, static_cast<std::uint32_t>(1u),
			2u, arr, false, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);

	expect_throw_pe_error([&instance] {
		(void)byte_vector_from_va(instance,
			static_cast<std::uint64_t>(1u), 2u, false, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);

	expect_throw_pe_error([&instance] {
		pe_bliss::packed_byte_vector arr;
		(void)byte_vector_from_va(instance, static_cast<std::uint64_t>(1u),
			2u, arr, false, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);
}

INSTANTIATE_TEST_SUITE_P(
	ByteVectorFromVaTests,
	ByteVectorFromVaFixture,
	::testing::Values(
		function_type::rva,
		function_type::va32,
		function_type::va64
	));
