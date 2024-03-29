#pragma once

#include <array>
#include <cstdint>

#include "gtest/gtest.h"

#include "pe_bliss2/image/image.h"

#include "tests/pe_bliss2/image_helper.h"

enum class function_type
{
	rva,
	va32,
	va64
};

class BytesToVaFixtureBase : public ::testing::TestWithParam<function_type>
{
public:
	static constexpr std::array header_arr{ std::byte{1}, std::byte{2}, std::byte{3} };
	static constexpr std::array section_arr{ std::byte{4}, std::byte{5}, std::byte{6} };
	std::uint32_t header_arr_offset{};
	std::uint32_t section_arr_offset{};
	std::uint32_t section_arr_rva{};

public:
	explicit BytesToVaFixtureBase()
		: instance(create_test_image({}))
	{
		instance.update_full_headers_buffer();
		auto& section = instance.get_section_data_list()[1];
		section_arr_offset = static_cast<std::uint32_t>(section.physical_size()
			- section_arr.size());
		test_image_options opts;
		section_arr_rva = opts.sections[0].virtual_size
			+ opts.start_section_rva + section_arr_offset;

		header_arr_offset = static_cast<std::uint32_t>(
			instance.get_full_headers_buffer().physical_size() - header_arr.size());
	}

	void check_section_data_equals() const
	{
		const auto& section = instance.get_section_data_list()[1];
		const auto& section_data = section.copied_data();
		const std::array actual_data{
			section_data[section_arr_offset],
			section_data[section_arr_offset + 1],
			section_data[section_arr_offset + 2]
		};
		EXPECT_EQ(actual_data, section_arr);
	}

	void check_header_data_equals(std::uint32_t actual_offset) const
	{
		const auto& data = instance.get_full_headers_buffer().copied_data();
		const std::array actual_data{
			data[actual_offset],
			data[actual_offset + 1],
			data[actual_offset + 2]
		};
		EXPECT_EQ(actual_data, header_arr);
	}

	void TestBody() override {}

public:
	pe_bliss::image::image instance;
};
