#include "gtest/gtest.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>

#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/image/byte_array_from_va.h"
#include "pe_bliss2/packed_byte_array.h"

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_errc.h"
#include "pe_bliss2/pe_types.h"

#include "tests/tests/pe_bliss2/image_helper.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

#include "utilities/generic_error.h"

namespace
{

enum class function_type
{
	rva,
	va32,
	va64
};

class ByteArrayFromVaFixture : public ::testing::TestWithParam<function_type>
{
public:
	static constexpr std::array header_arr{ std::byte{1}, std::byte{2}, std::byte{3} };
	static constexpr std::array section_arr{ std::byte{4}, std::byte{5}, std::byte{6} };
	static constexpr std::uint32_t header_arr_offset = 30u;
	std::uint32_t section_arr_offset{};
	std::uint32_t section_arr_rva{};

public:
	explicit ByteArrayFromVaFixture()
		: instance(create_test_image({}))
	{
		auto& section = instance.get_section_data_list()[1];
		auto& section_data = section.copied_data();
		instance.update_full_headers_buffer();
		auto& headers_data = instance.get_full_headers_buffer().copied_data();

		auto offset = header_arr_offset;
		for (auto byte : header_arr)
			headers_data[offset++] = byte;

		offset = static_cast<std::uint32_t>(section.size() - section_arr.size());
		section_arr_offset = offset;
		for (auto byte : section_arr)
			section_data[offset++] = byte;

		test_image_options opts;
		section_arr_rva = opts.sections[0].virtual_size
			+ opts.start_section_rva + section_arr_offset;
	}

	template<std::size_t MaxSize>
	decltype(auto) byte_array_from_address(pe_bliss::rva_type rva,
		std::size_t size, bool include_headers, bool allow_virtual_data)
	{
		switch (GetParam())
		{
		case function_type::va32:
			return byte_array_from_va<MaxSize>(instance, static_cast<std::uint32_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				static_cast<std::uint32_t>(size), include_headers, allow_virtual_data);
		case function_type::va64:
			return byte_array_from_va<MaxSize>(instance, static_cast<std::uint64_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				static_cast<std::uint32_t>(size), include_headers, allow_virtual_data);
		default: //rva
			return byte_array_from_rva<MaxSize>(instance, rva,
				static_cast<std::uint32_t>(size), include_headers, allow_virtual_data);
		}
	}

	template<std::size_t MaxSize>
	void byte_array_from_address(pe_bliss::rva_type rva,
		std::size_t size, pe_bliss::packed_byte_array<MaxSize>& arr,
		bool include_headers, bool allow_virtual_data)
	{
		switch (GetParam())
		{
		case function_type::va32:
			byte_array_from_va(instance, static_cast<std::uint32_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				static_cast<std::uint32_t>(size), arr, include_headers, allow_virtual_data);
			break;
		case function_type::va64:
			byte_array_from_va(instance, static_cast<std::uint64_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				static_cast<std::uint32_t>(size), arr, include_headers, allow_virtual_data);
			break;
		default: //rva
			byte_array_from_rva(instance, rva,
				static_cast<std::uint32_t>(size), arr, include_headers, allow_virtual_data);
			break;
		}
	}

	template<std::size_t NewSize, typename Arr>
	static auto extend_array(const Arr& arr)
	{
		static_assert(NewSize >= std::tuple_size_v<Arr>);
		std::array<typename Arr::value_type, NewSize> result{};
		auto it = result.begin();
		for (auto val : arr)
			*it++ = val;
		return result;
	}

public:
	pe_bliss::image::image instance;
};

} //namespace

TEST_P(ByteArrayFromVaFixture, PackedByteArraySectionTest)
{
	auto arr1 = byte_array_from_address<10u>(
		section_arr_rva, section_arr.size(), false, false);
	pe_bliss::packed_byte_array<10u> arr2;
	byte_array_from_address(
		section_arr_rva, section_arr.size(), arr2, false, false);

	for (const auto& arr : { arr1, arr2 })
	{
		EXPECT_EQ(arr.value(), extend_array<10u>(section_arr));
		EXPECT_EQ(arr.get_state().relative_offset(), section_arr_offset);
		EXPECT_EQ(arr.get_state().absolute_offset(), section_arr_offset);
		EXPECT_EQ(arr.get_state().buffer_pos(), 0u);
		EXPECT_EQ(arr.physical_size(), section_arr.size());
		EXPECT_EQ(arr.data_size(), section_arr.size());
		EXPECT_FALSE(arr.is_virtual());
	}
}

TEST_P(ByteArrayFromVaFixture, PackedByteArraySectionErrorTest)
{
	expect_throw_pe_error([this] {
		(void)byte_array_from_address<2u>(
			section_arr_rva, section_arr.size(), false, false);
	}, utilities::generic_errc::buffer_overrun);

	expect_throw_pe_error([this] {
		pe_bliss::packed_byte_array<2u> arr;
		(void)byte_array_from_address(
			section_arr_rva, section_arr.size(), arr, false, false);
	}, utilities::generic_errc::buffer_overrun);
}

TEST_P(ByteArrayFromVaFixture, PackedByteArrayHeaderErrorTest)
{
	expect_throw_pe_error([this] {
		(void)byte_array_from_address<10u>(
			header_arr_offset, header_arr.size(), false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	expect_throw_pe_error([this] {
		pe_bliss::packed_byte_array<10u> arr;
		(void)byte_array_from_address(
			header_arr_offset, header_arr.size(), arr, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST_P(ByteArrayFromVaFixture, PackedByteArrayHeaderTest)
{
	auto arr1 = byte_array_from_address<10u>(
		header_arr_offset, header_arr.size(), true, false);
	pe_bliss::packed_byte_array<10u> arr2;
	byte_array_from_address(
		header_arr_offset, header_arr.size(), arr2, true, false);

	for (const auto& arr : { arr1, arr2 })
	{
		EXPECT_EQ(arr.value(), extend_array<10u>(header_arr));
		EXPECT_EQ(arr.get_state().relative_offset(), header_arr_offset);
		EXPECT_EQ(arr.get_state().absolute_offset(), header_arr_offset);
		EXPECT_EQ(arr.get_state().buffer_pos(), 0u);
		EXPECT_EQ(arr.physical_size(), header_arr.size());
		EXPECT_EQ(arr.data_size(), header_arr.size());
		EXPECT_FALSE(arr.is_virtual());
	}
}

TEST_P(ByteArrayFromVaFixture, PackedByteArrayCutErrorTest)
{
	expect_throw_pe_error([this] {
		(void)byte_array_from_address<10u>(
			section_arr_offset, section_arr.size() + 1u, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	expect_throw_pe_error([this] {
		pe_bliss::packed_byte_array<10u> arr;
		(void)byte_array_from_address(
			section_arr_offset, section_arr.size() + 1u, arr, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST_P(ByteArrayFromVaFixture, PackedByteArraySectionVirtualDataTest)
{
	expect_throw_pe_error([this] {
		pe_bliss::packed_byte_array<10u> arr;
		(void)byte_array_from_address(
			section_arr_offset, section_arr.size() + 1u, arr, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
	auto arr1 = byte_array_from_address<10u>(
		section_arr_rva, section_arr.size() + 1u, false, true);
	pe_bliss::packed_byte_array<10u> arr2;
	byte_array_from_address(
		section_arr_rva, section_arr.size() + 1u, arr2, false, true);

	for (const auto& arr : { arr1, arr2 })
	{
		EXPECT_EQ(arr.value(), extend_array<10u>(section_arr));
		EXPECT_EQ(arr.get_state().relative_offset(), section_arr_offset);
		EXPECT_EQ(arr.get_state().absolute_offset(), section_arr_offset);
		EXPECT_EQ(arr.get_state().buffer_pos(), 0u);
		EXPECT_EQ(arr.physical_size(), section_arr.size());
		EXPECT_EQ(arr.data_size(), section_arr.size() + 1u);
		EXPECT_TRUE(arr.is_virtual());
	}
}

TEST(ByteArrayFromVa, PackedByteArrayFromVaErrorTest)
{
	auto instance = create_test_image({});

	expect_throw_pe_error([&instance] {
		(void)byte_array_from_va<10u>(instance,
			static_cast<std::uint32_t>(1u), 2u, false, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);

	expect_throw_pe_error([&instance] {
		pe_bliss::packed_byte_array<10u> arr;
		(void)byte_array_from_va(instance, static_cast<std::uint32_t>(1u),
			2u, arr, false, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);

	expect_throw_pe_error([&instance] {
		(void)byte_array_from_va<10u>(instance,
			static_cast<std::uint64_t>(1u), 2u, false, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);

	expect_throw_pe_error([&instance] {
		pe_bliss::packed_byte_array<10u> arr;
		(void)byte_array_from_va(instance, static_cast<std::uint64_t>(1u),
			2u, arr, false, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);
}

INSTANTIATE_TEST_SUITE_P(
	ByteArrayFromVaTests,
	ByteArrayFromVaFixture,
	::testing::Values(
		function_type::rva,
		function_type::va32,
		function_type::va64
	));
