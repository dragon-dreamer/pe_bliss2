#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <system_error>
#include <type_traits>
#include <utility>

#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_errc.h"
#include "pe_bliss2/image/string_to_va.h"
#include "pe_bliss2/packed_c_string.h"
#include "pe_bliss2/packed_utf16_string.h"
#include "pe_bliss2/pe_types.h"

#include "tests/tests/pe_bliss2/image_helper.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

namespace
{

enum class function_type
{
	rva,
	va32,
	va64
};

template<typename CharType>
class StringToVaFixture : public ::testing::Test
{
public:
	static constexpr std::uint32_t buffer_absolute_offset = 0x12u;
	using string_type = std::conditional_t<std::is_same_v<CharType, char>,
		pe_bliss::packed_c_string,
		pe_bliss::packed_utf16_string>;

	static constexpr std::array<CharType, 4> header_str = { 'a', 'b', 'c', 0 };
	static constexpr std::array<CharType, 4> section_str = { 'd', 'e', 'f', 0 };
	std::uint32_t header_arr_offset{};
	std::uint32_t section_arr_offset{};
	std::uint32_t section_arr_rva{};

	StringToVaFixture()
		: instance(create_test_image(test_image_options()))
	{
		instance.update_full_headers_buffer();
		auto& section = instance.get_section_data_list()[1];
		section_arr_offset = static_cast<std::uint32_t>(section.physical_size()
			- section_str.size() * sizeof(CharType));
		test_image_options opts;
		section_arr_rva = opts.sections[0].virtual_size
			+ opts.start_section_rva + section_arr_offset;

		header_arr_offset = static_cast<std::uint32_t>(
			instance.get_full_headers_buffer().physical_size()
			- header_str.size() * sizeof(CharType));
	}

	std::uint32_t string_to_address(function_type func_type, pe_bliss::rva_type rva,
		const string_type& str, bool include_headers, bool write_virtual_part)
	{
		auto image_base = instance.get_optional_header().get_raw_image_base();
		switch (func_type)
		{
		case function_type::va32:
			return static_cast<std::uint32_t>(string_to_va(
				instance, static_cast<std::uint32_t>(rva
					+ image_base), str, include_headers, write_virtual_part)
				- image_base);
		case function_type::va64:
			return static_cast<std::uint32_t>(string_to_va(
				instance, static_cast<std::uint64_t>(rva
					+ image_base), str, include_headers, write_virtual_part)
				- image_base);
		default: //rva
			return string_to_rva(instance, rva, str, include_headers,
				write_virtual_part);
		}
	}

	template<typename Func>
	void test_each_overload(Func&& func)
	{
		for (auto func_type : { function_type::rva,
			function_type::va32, function_type::va64 })
		{
			std::forward<Func>(func)(function_type::rva);
			auto& header_range = instance.get_full_headers_buffer().copied_data();
			std::fill(header_range.begin(), header_range.end(), std::byte{});
			auto& section_range = instance.get_section_data_list()[1].copied_data();
			std::fill(section_range.begin(), section_range.end(), std::byte{});
		}
	}

	template<typename Str>
	static auto create_string(const Str& str)
	{
		string_type result;
		result.value() = str.data();
		if constexpr (!std::is_same_v<CharType, char>)
			result.sync_physical_size();
		return result;
	}

	template<typename It, typename Arr>
	void check_string(It begin, const Arr& arr) const
	{
		if constexpr (std::is_same_v<CharType, char>)
		{
			auto byte_arr = reinterpret_cast<const std::byte*>(arr.data());
			EXPECT_TRUE(std::equal(byte_arr, byte_arr + arr.size(), begin));
		}
		else
		{
			EXPECT_EQ(static_cast<std::size_t>(*begin++), arr.size() - 1);
			EXPECT_EQ(*begin++, std::byte{});

			for (auto it = arr.begin(), end = arr.end() - 1; it != end; ++it) {
				EXPECT_EQ(static_cast<std::byte>(*it & 0xffu), *begin++);
				EXPECT_EQ(*begin++, std::byte{});
			}
		}
	}

	void check_header_string(std::uint32_t offset) const
	{
		check_string(instance.get_full_headers_buffer().copied_data().begin()
			+ offset, header_str);
	}

	void check_section_string() const
	{
		check_string(instance.get_section_data_list()[1].copied_data().begin()
			+ section_arr_offset, section_str);
	}

	void TestBody() {}

public:
	pe_bliss::image::image instance;
};

} //namespace

using string_to_va_types = testing::Types<char, char16_t>;
TYPED_TEST_SUITE(StringToVaFixture, string_to_va_types);

TYPED_TEST(StringToVaFixture, StringToSectionTest)
{
	this->test_each_overload([this] (function_type func_type) {
		auto str = this->create_string(this->section_str);
		EXPECT_EQ(this->string_to_address(func_type,
			this->section_arr_rva, str, false, false),
			this->section_arr_rva + str.physical_size());
		this->check_section_string();
	});
}

TYPED_TEST(StringToVaFixture, StringToHeaderTest)
{
	this->test_each_overload([this](function_type func_type) {
		auto str = this->create_string(this->header_str);
		EXPECT_EQ(this->string_to_address(func_type,
			this->header_arr_offset, str, true, false),
			this->header_arr_offset + str.physical_size());
		this->check_header_string(this->header_arr_offset);
	});
}

TYPED_TEST(StringToVaFixture, StringToHeaderErrorTest)
{
	this->test_each_overload([this](function_type func_type) {
		auto str = this->create_string(this->header_str);

		expect_throw_pe_error([this, &str, func_type] {
			this->string_to_address(func_type,
				this->header_arr_offset, str, false, false);
		}, pe_bliss::image::image_errc::section_data_does_not_exist);
	});
}

TEST(StringToVaTests, StringToSectionVirtualDataTest)
{
	StringToVaFixture<char16_t> fixture;
	auto str = fixture.create_string(fixture.section_str);
	str.set_data_size(str.physical_size() + 2u);
	fixture.test_each_overload([this, &str, &fixture](function_type func_type) {
		EXPECT_THROW((void)fixture.string_to_address(func_type,
			fixture.section_arr_rva, str, false, true), std::system_error);
	});
}

TYPED_TEST(StringToVaFixture, StringToSectionVirtualAddressConversionErrorTest)
{
	auto str = this->create_string(this->section_str);
	expect_throw_pe_error([this, &str] {
		string_to_va(this->instance, static_cast<std::uint32_t>(1u), str, true, true);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);
	expect_throw_pe_error([this, &str] {
		string_to_va(this->instance, static_cast<std::uint64_t>(1u), str, true, true);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);
}

TYPED_TEST(StringToVaFixture, StringToFileOffsetHeaderErrorTest)
{
	auto str = this->create_string(this->header_str);
	expect_throw_pe_error([this, &str]
	{
		(void)string_to_file_offset(this->instance, str, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TYPED_TEST(StringToVaFixture, StringToFileOffsetHeaderTest)
{
	auto str = this->create_string(this->header_str);
	str.get_state().set_absolute_offset(this->buffer_absolute_offset);
	EXPECT_EQ(string_to_file_offset(this->instance, str, true, false),
		this->buffer_absolute_offset + str.physical_size());
	this->check_header_string(this->buffer_absolute_offset);
}
