#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>

#include "gtest/gtest.h"

#include "pe_bliss2/image/image.h"

#include "tests/pe_bliss2/image_helper.h"

enum class function_type
{
	rva,
	va32,
	va64
};

class ByteContainerFromVaFixture : public ::testing::TestWithParam<function_type>
{
public:
	static constexpr std::array header_arr{ std::byte{1}, std::byte{2}, std::byte{3} };
	static constexpr std::array section_arr{ std::byte{4}, std::byte{5}, std::byte{6} };
	static constexpr std::uint32_t header_arr_offset = 30u;
	std::uint32_t section_arr_offset{};
	std::uint32_t section_arr_rva{};

public:
	explicit ByteContainerFromVaFixture()
		: instance(create_test_image({}))
	{
		auto& section = instance.get_section_data_list()[1];
		auto& section_data = section.copied_data();
		instance.update_full_headers_buffer();
		auto& headers_data = instance.get_full_headers_buffer().copied_data();

		auto offset = header_arr_offset;
		for (auto byte : header_arr)
			headers_data[offset++] = byte;

		offset = static_cast<std::uint32_t>(
			section.physical_size() - section_arr.size());
		section_arr_offset = offset;
		for (auto byte : section_arr)
			section_data[offset++] = byte;

		test_image_options opts;
		section_arr_rva = opts.sections[0].virtual_size
			+ opts.start_section_rva + section_arr_offset;
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
