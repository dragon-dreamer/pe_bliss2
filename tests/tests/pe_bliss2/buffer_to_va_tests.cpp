#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <tuple>
#include <system_error>

#include "pe_bliss2/address_converter.h"
#include "buffers/input_container_buffer.h"
#include "buffers/ref_buffer.h"

#include "pe_bliss2/image/buffer_to_va.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_errc.h"
#include "pe_bliss2/pe_types.h"

#include "tests/tests/pe_bliss2/bytes_to_va_fixture_base.h"
#include "tests/tests/pe_bliss2/image_helper.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

namespace
{

class BufferToVaFixture : public BytesToVaFixtureBase
{
public:
	static constexpr std::uint32_t buffer_absolute_offset = 0x12u;

	std::uint32_t buffer_to_address(pe_bliss::rva_type rva,
		const buffers::ref_buffer& buf, bool include_headers)
	{
		auto image_base = instance.get_optional_header().get_raw_image_base();
		switch (GetParam())
		{
		case function_type::va32:
			return static_cast<std::uint32_t>(buffer_to_va(
				instance, static_cast<std::uint32_t>(rva
				+ image_base), buf, include_headers) - image_base);
		case function_type::va64:
			return static_cast<std::uint32_t>(buffer_to_va(
				instance, static_cast<std::uint64_t>(rva
				+ image_base), buf, include_headers) - image_base);
		default: //rva
			return buffer_to_rva(instance, rva, buf, include_headers);
		}
	}

	template<typename Arr>
	static buffers::ref_buffer create_buffer(const Arr& arr)
	{
		auto buf = std::make_shared<buffers::input_container_buffer>();
		buf->set_absolute_offset(buffer_absolute_offset);
		buf->get_container() = std::vector(arr.begin(), arr.end());
		buffers::ref_buffer result;
		result.deserialize(buf, false);
		return result;
	}
};

} //namespace

TEST_P(BufferToVaFixture, BufferToSectionTest)
{
	EXPECT_EQ(buffer_to_address(section_arr_rva, create_buffer(section_arr), false),
		section_arr_rva + section_arr.size());
	check_section_data_equals();
}

TEST_P(BufferToVaFixture, BufferToHeaderTest)
{
	EXPECT_EQ(buffer_to_address(header_arr_offset, create_buffer(header_arr), true),
		header_arr_offset + header_arr.size());
	check_header_data_equals(header_arr_offset);
}

TEST_P(BufferToVaFixture, BufferToHeaderErrorTest)
{
	expect_throw_pe_error([this]
	{
		(void)buffer_to_address(header_arr_offset,
			create_buffer(header_arr), false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST_P(BufferToVaFixture, BufferToSectionOverflowErrorTest)
{
	std::array<std::byte, std::tuple_size_v<decltype(section_arr)> +1u> extended{};
	std::copy(section_arr.begin(), section_arr.end(), extended.begin());
	EXPECT_THROW((void)buffer_to_address(section_arr_rva,
		create_buffer(extended), false), std::system_error);
}

TEST(BufferToVaTests, BufferToVaAddressConversionErrorTest)
{
	auto instance = create_test_image({});
	auto buf = BufferToVaFixture::create_buffer(BufferToVaFixture::section_arr);

	expect_throw_pe_error([&instance, &buf] {
		(void)buffer_to_va(instance,
			static_cast<std::uint32_t>(1u), buf, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);

	expect_throw_pe_error([&instance, &buf] {
		(void)buffer_to_va(instance,
			static_cast<std::uint64_t>(1u), buf, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);
}

TEST(BufferToVaTests, BufferToFileOffsetHeaderErrorTest)
{
	auto instance = create_test_image({});
	instance.update_full_headers_buffer();
	auto buf = BufferToVaFixture::create_buffer(BufferToVaFixture::header_arr);
	expect_throw_pe_error([&instance, &buf]
	{
		(void)buffer_to_file_offset(instance, buf, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST(BufferToVaTest, BufferToFileOffsetHeaderTest)
{
	BufferToVaFixture fixture;
	auto buf = fixture.create_buffer(fixture.header_arr);
	EXPECT_EQ(buffer_to_file_offset(fixture.instance, buf, true),
		fixture.buffer_absolute_offset + fixture.header_arr.size());
	fixture.check_header_data_equals(fixture.buffer_absolute_offset);
}

INSTANTIATE_TEST_SUITE_P(
	BufferToVaTests,
	BufferToVaFixture,
	::testing::Values(
		function_type::rva,
		function_type::va32,
		function_type::va64
	));
