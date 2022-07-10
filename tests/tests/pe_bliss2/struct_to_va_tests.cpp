#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <system_error>

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_errc.h"
#include "pe_bliss2/image/struct_to_va.h"
#include "pe_bliss2/pe_types.h"
#include "pe_bliss2/packed_struct.h"

#include "pe_bliss2/address_converter.h"
#include "tests/tests/pe_bliss2/image_helper.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

namespace
{

struct test_struct
{
	std::uint8_t value1;
	std::uint8_t value2;
};

constexpr std::size_t test_struct_size = 2u;

enum class function_type
{
	rva,
	va32,
	va64
};

class StructToVaFixture : public ::testing::TestWithParam<function_type>
{
public:
	static constexpr std::uint32_t buffer_absolute_offset = 0x12u;

	static constexpr test_struct test_struct_obj{ 0xaa, 0xbb };

	std::uint32_t header_struct_offset{};
	std::uint32_t section_struct_offset{};
	std::uint32_t section_struct_rva{};

	StructToVaFixture()
		: instance(create_test_image({}))
	{
		instance.update_full_headers_buffer();
		auto& section = instance.get_section_data_list()[1];
		section_struct_offset = static_cast<std::uint32_t>(section.size()
			- test_struct_size);
		test_image_options opts;
		section_struct_rva = opts.sections[0].virtual_size
			+ opts.start_section_rva + section_struct_offset;

		header_struct_offset = static_cast<std::uint32_t>(
			instance.get_full_headers_buffer().size()
			- test_struct_size);
	}

	template<typename Struct>
	std::uint32_t struct_to_address(function_type func_type, pe_bliss::rva_type rva,
		const Struct& obj, bool include_headers, bool write_virtual_part,
		bool cut_if_does_not_fit)
	{
		auto image_base = instance.get_optional_header().get_raw_image_base();
		switch (func_type)
		{
		case function_type::va32:
			return static_cast<std::uint32_t>(struct_to_va(
				instance, static_cast<std::uint32_t>(rva
					+ image_base), obj, include_headers, write_virtual_part,
					cut_if_does_not_fit)
				- image_base);
		case function_type::va64:
			return static_cast<std::uint32_t>(struct_to_va(
				instance, static_cast<std::uint64_t>(rva
					+ image_base), obj, include_headers, write_virtual_part,
					cut_if_does_not_fit)
				- image_base);
		default: //rva
			return struct_to_rva(instance, rva, obj, include_headers,
				write_virtual_part, cut_if_does_not_fit);
		}
	}

	template<typename It>
	void check_struct(It begin) const
	{
		EXPECT_EQ(*begin++, std::byte{ test_struct_obj.value1 });
		EXPECT_EQ(*begin, std::byte{ test_struct_obj.value2 });
	}

	void check_header_struct(std::uint32_t offset) const
	{
		check_struct(instance.get_full_headers_buffer().copied_data().begin()
			+ offset);
	}

	void check_section_struct() const
	{
		check_struct(instance.get_section_data_list()[1].copied_data().begin()
			+ section_struct_offset);
	}
	
	static pe_bliss::packed_struct<test_struct> create_struct()
	{
		return test_struct_obj;
	}

	void TestBody() {}

public:
	pe_bliss::image::image instance;
};

} //namespace

TEST_P(StructToVaFixture, StructToSectionTest)
{
	EXPECT_EQ(struct_to_address(GetParam(), section_struct_rva, create_struct(),
		false, false, false), section_struct_rva + test_struct_size);
	check_section_struct();
}

TEST_P(StructToVaFixture, StructToSectionEmptyTest)
{
	auto obj = create_struct();
	obj.set_physical_size(0u);
	EXPECT_EQ(struct_to_address(GetParam(), 0xffffffffu, obj,
		false, false, false), 0xffffffffu);
}

TEST_P(StructToVaFixture, StructToHeaderTest)
{
	EXPECT_EQ(struct_to_address(GetParam(), header_struct_offset, create_struct(),
		true, false, false), header_struct_offset + test_struct_size);
	check_header_struct(header_struct_offset);
}

TEST_P(StructToVaFixture, StructToHeaderErrorTest)
{
	expect_throw_pe_error([this]
	{
		struct_to_address(GetParam(), header_struct_offset, create_struct(),
			false, false, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST_P(StructToVaFixture, StructToSectionVirtualTest)
{
	auto obj = create_struct();
	obj.set_physical_size(test_struct_size - 1u);
	EXPECT_EQ(struct_to_address(GetParam(), section_struct_rva, obj,
		false, true, false), section_struct_rva + test_struct_size);
	check_section_struct();
}

TEST_P(StructToVaFixture, StructToSectionVirtualPartialTest)
{
	auto obj = create_struct();
	obj.set_physical_size(test_struct_size - 1u);
	EXPECT_EQ(struct_to_address(GetParam(), section_struct_rva, obj,
		false, false, false), section_struct_rva + test_struct_size - 1u);
	auto it = instance.get_section_data_list()[1].copied_data().begin()
		+ section_struct_offset;
	EXPECT_EQ(*it++, std::byte{ test_struct_obj.value1 });
	EXPECT_EQ(*it, std::byte{});
}

TEST_P(StructToVaFixture, StructToSectionNoCutErrorTest)
{
	EXPECT_THROW(struct_to_address(GetParam(),
		section_struct_rva + 1u, create_struct(),
		false, false, false), std::system_error);
}

TEST_P(StructToVaFixture, StructToSectionCutTest)
{
	EXPECT_EQ(struct_to_address(GetParam(),
		section_struct_rva + 1u, create_struct(),
		false, false, true), section_struct_rva + test_struct_size);
	auto it = instance.get_section_data_list()[1].copied_data().begin()
		+ section_struct_offset;
	EXPECT_EQ(*it++, std::byte{});
	EXPECT_EQ(*it, std::byte{ test_struct_obj.value1 });
}

TEST(StructToVaTests, StructToVaAddressConversionErrorTest)
{
	auto instance = create_test_image({});
	auto obj = StructToVaFixture::create_struct();

	expect_throw_pe_error([&instance, &obj] {
		(void)struct_to_va(instance,
			static_cast<std::uint32_t>(1u), obj, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);

	expect_throw_pe_error([&instance, &obj] {
		(void)struct_to_va(instance,
			static_cast<std::uint64_t>(1u), obj, false);
	}, pe_bliss::address_converter_errc::address_conversion_overflow);
}

TEST(StructToVaTests, StructToFileOffsetHeaderErrorTest)
{
	auto instance = create_test_image({});
	instance.update_full_headers_buffer();
	auto obj = StructToVaFixture::create_struct();
	expect_throw_pe_error([&instance, &obj]
	{
		(void)struct_to_file_offset(instance, obj, false);
	}, pe_bliss::image::image_errc::section_data_does_not_exist);
}

TEST(StructToVaTests, StructToFileOffsetHeaderTest)
{
	StructToVaFixture fixture;
	auto obj = fixture.create_struct();
	obj.get_state().set_absolute_offset(fixture.buffer_absolute_offset);
	EXPECT_EQ(struct_to_file_offset(fixture.instance, obj, true),
		fixture.buffer_absolute_offset + test_struct_size);
	fixture.check_header_struct(fixture.buffer_absolute_offset);
}

INSTANTIATE_TEST_SUITE_P(
	StructToVaTests,
	StructToVaFixture,
	::testing::Values(
		function_type::rva,
		function_type::va32,
		function_type::va64
	));
