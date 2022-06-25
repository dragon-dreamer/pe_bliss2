#include "gtest/gtest.h"

#include <climits>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "buffers/ref_buffer.h"

#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_errc.h"
#include "pe_bliss2/image/string_from_va.h"
#include "pe_bliss2/packed_c_string.h"
#include "pe_bliss2/packed_string_type.h"
#include "pe_bliss2/packed_utf16_string.h"
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

template<typename String>
class StringFromVaFixtureBase : public ::testing::TestWithParam<function_type>
{
public:
	using string_type = String;

public:
	explicit StringFromVaFixtureBase(std::uint32_t section_string_rva)
		: instance(create_test_image({}))
		, section_string_rva(section_string_rva)
	{
	}

	decltype(auto) string_from_address(pe_bliss::rva_type rva,
		bool include_headers, bool allow_virtual_data)
	{
		switch (GetParam())
		{
		case function_type::va32:
			return string_from_va<String>(instance,
				static_cast<std::uint32_t>(rva
					+ instance.get_optional_header().get_raw_image_base()),
				include_headers, allow_virtual_data);
		case function_type::va64:
			return string_from_va<String>(instance,
				static_cast<std::uint64_t>(rva
					+ instance.get_optional_header().get_raw_image_base()),
				include_headers, allow_virtual_data);
		default: //rva
			return string_from_rva<String>(instance, rva,
				include_headers, allow_virtual_data);
		}
	}

	decltype(auto) string_from_address(pe_bliss::rva_type rva, String& str,
		bool include_headers, bool allow_virtual_data)
	{
		switch (GetParam())
		{
		case function_type::va32:
			return string_from_va(instance, static_cast<std::uint32_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				str, include_headers, allow_virtual_data);
		case function_type::va64:
			return string_from_va(instance, static_cast<std::uint64_t>(rva
				+ instance.get_optional_header().get_raw_image_base()),
				str, include_headers, allow_virtual_data);
		default: //rva
			return string_from_rva(instance, rva,
				str, include_headers, allow_virtual_data);
		}
	}

protected:
	static pe_bliss::rva_type get_section_string_rva(uint32_t section_string_offset)
	{
		test_image_options opts;
		return opts.sections[0].virtual_size
			+ opts.start_section_rva + section_string_offset;
	}

	static void write_string(buffers::ref_buffer::container_type& data,
		std::uint32_t offset, std::string_view str)
	{
		for (auto ch : str)
			data[offset++] = std::byte{ static_cast<std::uint8_t>(ch) };
	}

	static void write_string(buffers::ref_buffer::container_type& data,
		std::uint32_t offset, std::u16string_view str)
	{
		for (auto ch : str)
		{
			data[offset++] = std::byte{ static_cast<std::uint8_t>(ch) };
			data[offset++] = std::byte{ static_cast<std::uint8_t>(ch >> CHAR_BIT) };
		}
	}

	static void write_length(buffers::ref_buffer::container_type& data,
		std::uint16_t length, std::uint32_t& offset)
	{
		data[offset++] = std::byte{ static_cast<std::uint8_t>(length) };
		data[offset++] = std::byte{ static_cast<std::uint8_t>(length >> CHAR_BIT) };
	}

public:
	pe_bliss::image::image instance;
	pe_bliss::rva_type section_string_rva{};
	pe_bliss::rva_type cut_string_rva{};
	std::uint32_t cut_string_offset{};
};

class CStringFromVaFixture : public StringFromVaFixtureBase<pe_bliss::packed_c_string>
{
public:
	static constexpr std::string_view header_string{ "header_string" };
	static constexpr std::string_view section_string{ "test_string" };
	static constexpr std::string_view cut_string{ "cut" };
	static constexpr std::uint32_t header_string_offset = 30u;
	static constexpr std::uint32_t section_string_offset = 20u;

public:
	CStringFromVaFixture()
		: StringFromVaFixtureBase(get_section_string_rva(section_string_offset))
	{
		auto& section = instance.get_section_data_list()[1];
		auto& data = section.copied_data();
		instance.update_full_headers_buffer();
		auto& headers_data = instance.get_full_headers_buffer().copied_data();

		auto offset = section_string_offset;
		write_string(data, offset, section_string);

		offset = header_string_offset;
		write_string(headers_data, offset, header_string);

		offset = static_cast<std::uint32_t>(section.size() - cut_string.size());
		cut_string_offset = offset;
		cut_string_rva = get_section_string_rva(offset);
		write_string(data, offset, cut_string);
	}

	std::size_t expected_physical_size(std::string_view str,
		bool virtual_nullbyte = false)
	{
		return str.size()
			+ (virtual_nullbyte ? 0u : sizeof(std::string_view::value_type));
	}
};

class U16StringFromVaFixture : public StringFromVaFixtureBase<pe_bliss::packed_utf16_string>
{
public:
	static constexpr std::u16string_view header_string{ u"header_string" };
	static constexpr std::u16string_view section_string{ u"test_string" };
	static constexpr std::u16string_view cut_string{ u"cut" };
	static constexpr std::uint32_t header_string_offset = 50u;
	static constexpr std::uint32_t section_string_offset = 70u;
	static constexpr std::uint16_t cut_string_size = 10u;

public:
	U16StringFromVaFixture()
		: StringFromVaFixtureBase(get_section_string_rva(section_string_offset))
	{
		auto& section = instance.get_section_data_list()[1];
		auto& data = section.copied_data();
		instance.update_full_headers_buffer();
		auto& headers_data = instance.get_full_headers_buffer().copied_data();

		auto offset = section_string_offset;
		write_length(data, static_cast<std::uint16_t>(
			section_string.size()), offset);
		write_string(data, offset, section_string);

		offset = header_string_offset;
		write_length(headers_data, static_cast<std::uint16_t>(
			header_string.size()), offset);
		write_string(headers_data, offset, header_string);

		offset = static_cast<std::uint32_t>(section.size()
			- cut_string.size() * sizeof(char16_t))
			- sizeof(std::uint16_t) /* length encoded */;
		cut_string_offset = offset;
		cut_string_rva = get_section_string_rva(offset);
		write_length(data, cut_string_size, offset);
		write_string(data, offset, cut_string);
	}

	std::size_t expected_physical_size(std::u16string_view str,
		bool /* virtual_nullbyte */ = false)
	{
		return str.size() * sizeof(std::u16string_view::value_type)
			+ sizeof(std::uint16_t) /* length encoded*/;
	}
};

template<typename Fixture>
void test_with_fixture_section(Fixture& fixture)
{
	auto str1 = fixture.string_from_address(fixture.section_string_rva, false, false);
	typename Fixture::string_type str2;
	fixture.string_from_address(fixture.section_string_rva, str2, false, false);
	for (const auto& str : { str1, str2 })
	{
		EXPECT_EQ(str.value(), fixture.section_string);
		EXPECT_EQ(str.get_state().relative_offset(), fixture.section_string_offset);
		EXPECT_EQ(str.get_state().absolute_offset(), fixture.section_string_offset);
		EXPECT_EQ(str.get_state().buffer_pos(), 0u);
		EXPECT_EQ(str.physical_size(),
			fixture.expected_physical_size(fixture.section_string));
		EXPECT_FALSE(str.is_virtual());
	}
}

template<typename Fixture>
void test_with_fixture_header_error(Fixture& fixture)
{
	typename Fixture::string_type str;

	expect_throw_pe_error([&fixture] {
		(void)fixture.string_from_address(fixture.header_string_offset, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);

	expect_throw_pe_error([&fixture, &str] {
		(void)fixture.string_from_address(fixture.header_string_offset, str, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

template<typename Fixture>
void test_with_fixture_header(Fixture& fixture)
{
	auto str1 = fixture.string_from_address(fixture.header_string_offset, true, false);
	typename Fixture::string_type str2;
	fixture.string_from_address(fixture.header_string_offset, str2, true, false);
	for (const auto& str : { str1, str2 })
	{
		EXPECT_EQ(str.value(), fixture.header_string);
		EXPECT_EQ(str.get_state().relative_offset(), fixture.header_string_offset);
		EXPECT_EQ(str.get_state().absolute_offset(), fixture.header_string_offset);
		EXPECT_EQ(str.get_state().buffer_pos(), 0u);
		EXPECT_EQ(str.physical_size(),
			fixture.expected_physical_size(fixture.header_string));
		EXPECT_FALSE(str.is_virtual());
	}

	//TODO: test string_from_va address overflow exception
}

template<typename Fixture>
void test_with_fixture_cut_string_error(Fixture& fixture)
{
	typename Fixture::string_type str;

	expect_throw_pe_error([&fixture] {
		(void)fixture.string_from_address(fixture.cut_string_rva, false, false);
	}, utilities::generic_errc::buffer_overrun);
}

template<typename Fixture>
void test_with_fixture_cut_string(Fixture& fixture)
{
	auto str1 = fixture.string_from_address(fixture.cut_string_rva, false, true);
	typename Fixture::string_type str2;
	fixture.string_from_address(fixture.cut_string_rva, str2, false, true);
	for (const auto& str : { str1, str2 })
	{
		EXPECT_EQ(str.value(), fixture.cut_string);
		EXPECT_EQ(str.get_state().relative_offset(), fixture.cut_string_offset);
		EXPECT_EQ(str.get_state().absolute_offset(), fixture.cut_string_offset);
		EXPECT_EQ(str.get_state().buffer_pos(), 0u);
		EXPECT_EQ(str.physical_size(),
			fixture.expected_physical_size(fixture.cut_string, true));
		EXPECT_TRUE(str.is_virtual());
		if constexpr (std::is_same_v<typename Fixture::string_type,
			pe_bliss::packed_utf16_string>)
		{
			EXPECT_EQ(str.virtual_string_length(), fixture.cut_string_size);
		}
	}
}

} //namespace

//TODO
TEST_P(CStringFromVaFixture, PackedStringSectionTest)
{
	test_with_fixture_section(*this);
}

TEST_P(U16StringFromVaFixture, PackedStringSectionTest)
{
	test_with_fixture_section(*this);
}

TEST_P(CStringFromVaFixture, PackedStringHeaderErrorTest)
{
	test_with_fixture_header_error(*this);
}

TEST_P(U16StringFromVaFixture, PackedStringHeaderErrorTest)
{
	test_with_fixture_header_error(*this);
}

TEST_P(CStringFromVaFixture, PackedStringHeaderTest)
{
	test_with_fixture_header(*this);
}

TEST_P(U16StringFromVaFixture, PackedStringHeaderTest)
{
	test_with_fixture_header(*this);
}

TEST_P(CStringFromVaFixture, PackedStringCutStringErrorTest)
{
	test_with_fixture_cut_string_error(*this);
}

TEST_P(U16StringFromVaFixture, PackedStringCutStringErrorTest)
{
	test_with_fixture_cut_string_error(*this);
}

TEST_P(CStringFromVaFixture, PackedStringCutStringTest)
{
	test_with_fixture_cut_string(*this);
}

TEST_P(U16StringFromVaFixture, PackedStringCutStringTest)
{
	test_with_fixture_cut_string(*this);
}

TEST(StringFromVa, StringFromVaErrorTest)
{
	auto instance = create_test_image({});

	expect_throw_pe_error([&instance] {
		(void)string_from_va(instance,
			static_cast<std::uint32_t>(0x1u), true, true);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);

	expect_throw_pe_error([&instance] {
		(void)string_from_va(instance,
			static_cast<std::uint64_t>(0x1u), true, true);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);
}

INSTANTIATE_TEST_SUITE_P(
	CStringFromVaTests,
	CStringFromVaFixture,
	::testing::Values(
		function_type::rva,
		function_type::va32,
		function_type::va64
	));

INSTANTIATE_TEST_SUITE_P(
	U16StringFromVaTests,
	U16StringFromVaFixture,
	::testing::Values(
		function_type::rva,
		function_type::va32,
		function_type::va64
	));
