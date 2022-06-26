#include "gtest/gtest.h"

#include <cstdint>

#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_errc.h"
#include "pe_bliss2/image/struct_from_va.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/pe_types.h"

#include "tests/tests/pe_bliss2/byte_container_fixture_base.h"
#include "tests/tests/pe_bliss2/image_helper.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

#include "utilities/generic_error.h"

namespace
{

class StructFromVaFixture : public ByteContainerFromVaFixture
{
public:
	template<typename Struct>
	decltype(auto) struct_from_address(pe_bliss::rva_type rva,
		bool include_headers, bool allow_virtual_data)
	{
		switch (GetParam())
		{
		case function_type::va32:
			return struct_from_va<Struct>(instance, static_cast<std::uint32_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				include_headers, allow_virtual_data);
		case function_type::va64:
			return struct_from_va<Struct>(instance, static_cast<std::uint64_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				include_headers, allow_virtual_data);
		default: //rva
			return struct_from_rva<Struct>(instance, rva,
				include_headers, allow_virtual_data);
		}
	}

	template<typename Struct>
	void struct_from_address(pe_bliss::rva_type rva,
		Struct& value, bool include_headers, bool allow_virtual_data)
	{
		switch (GetParam())
		{
		case function_type::va32:
			struct_from_va(instance, static_cast<std::uint32_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				value, include_headers, allow_virtual_data);
			break;
		case function_type::va64:
			struct_from_va(instance, static_cast<std::uint64_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				value, include_headers, allow_virtual_data);
			break;
		default: //rva
			struct_from_rva(instance, rva,
				value, include_headers, allow_virtual_data);
			break;
		}
	}
};

struct test_struct
{
	std::byte a, b, c;
	friend bool operator==(const test_struct&,
		const test_struct&) = default;
};

struct test_virtual_struct
{
	std::byte a, b, c, d;
	friend bool operator==(const test_virtual_struct&,
		const test_virtual_struct&) = default;
};

using test_packed_struct = pe_bliss::packed_struct<test_struct>;
using test_packed_virtual_struct
	= pe_bliss::packed_struct<test_virtual_struct>;

} //namespace

TEST_P(StructFromVaFixture, PackedStructSectionTest)
{
	auto struct1 = struct_from_address<test_struct>(
		section_arr_rva, false, false);
	test_packed_struct struct2;
	struct_from_address(
		section_arr_rva, struct2, false, false);

	test_struct expected{ section_arr[0], section_arr[1], section_arr[2] };
	for (const auto& obj : { struct1, struct2 })
	{
		EXPECT_EQ(obj.get(), expected);
		EXPECT_EQ(obj.data_size(), section_arr.size());
		EXPECT_EQ(obj.physical_size(), section_arr.size());
		EXPECT_EQ(obj.get_state().relative_offset(), section_arr_offset);
		EXPECT_EQ(obj.get_state().absolute_offset(), section_arr_offset);
		EXPECT_EQ(obj.get_state().buffer_pos(), 0u);
		EXPECT_FALSE(obj.is_virtual());
	}
}

TEST_P(StructFromVaFixture, PackedStructHeaderErrorTest)
{
	expect_throw_pe_error([this] {
		(void)struct_from_address<test_struct>(
			header_arr_offset, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	expect_throw_pe_error([this] {
		test_packed_struct obj;
		struct_from_address(
			header_arr_offset, obj, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST_P(StructFromVaFixture, PackedStructHeaderTest)
{
	auto struct1 = struct_from_address<test_struct>(
		header_arr_offset, true, false);
	test_packed_struct struct2;
	struct_from_address(
		header_arr_offset, struct2, true, false);

	test_struct expected{ header_arr[0], header_arr[1], header_arr[2] };
	for (const auto& obj : { struct1, struct2 })
	{
		EXPECT_EQ(obj.get(), expected);
		EXPECT_EQ(obj.data_size(), header_arr.size());
		EXPECT_EQ(obj.physical_size(), header_arr.size());
		EXPECT_EQ(obj.get_state().relative_offset(), header_arr_offset);
		EXPECT_EQ(obj.get_state().absolute_offset(), header_arr_offset);
		EXPECT_EQ(obj.get_state().buffer_pos(), 0u);
		EXPECT_FALSE(obj.is_virtual());
	}
}

TEST_P(StructFromVaFixture, PackedStructCutErrorTest)
{
	expect_throw_pe_error([this] {
		(void)struct_from_address<test_virtual_struct>(
			section_arr_rva, false, false);
	}, utilities::generic_errc::buffer_overrun);

	expect_throw_pe_error([this] {
		test_packed_virtual_struct obj;
		struct_from_address(
			section_arr_rva, obj, false, false);
	}, utilities::generic_errc::buffer_overrun);
}

TEST_P(StructFromVaFixture, PackedStructSectionVirtualDataTest)
{
	auto struct1 = struct_from_address<test_virtual_struct>(
		section_arr_rva, false, true);
	test_packed_virtual_struct struct2;
	struct_from_address(
		section_arr_rva, struct2, false, true);

	test_virtual_struct expected{
		section_arr[0], section_arr[1], section_arr[2] };
	for (const auto& obj : { struct1, struct2 })
	{
		EXPECT_EQ(obj.get(), expected);
		EXPECT_EQ(obj.data_size(), section_arr.size() + 1u);
		EXPECT_EQ(obj.physical_size(), section_arr.size());
		EXPECT_EQ(obj.get_state().relative_offset(), section_arr_offset);
		EXPECT_EQ(obj.get_state().absolute_offset(), section_arr_offset);
		EXPECT_EQ(obj.get_state().buffer_pos(), 0u);
		EXPECT_TRUE(obj.is_virtual());
	}
}

TEST(StructFromVaFixture, PackedStructFromVaErrorTest)
{
	auto instance = create_test_image({});

	expect_throw_pe_error([&instance] {
		(void)struct_from_va<test_struct>(instance,
			static_cast<std::uint32_t>(1u), false, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);

	expect_throw_pe_error([&instance] {
		test_packed_struct obj;
		struct_from_va(instance, static_cast<std::uint32_t>(1u),
			obj, false, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);

	expect_throw_pe_error([&instance] {
		(void)struct_from_va<test_struct>(instance,
			static_cast<std::uint64_t>(1u), false, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);

	expect_throw_pe_error([&instance] {
		test_packed_struct obj;
		struct_from_va(instance, static_cast<std::uint64_t>(1u),
			obj, false, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);
}

INSTANTIATE_TEST_SUITE_P(
	StructFromVaTests,
	StructFromVaFixture,
	::testing::Values(
		function_type::rva,
		function_type::va32,
		function_type::va64
	));
