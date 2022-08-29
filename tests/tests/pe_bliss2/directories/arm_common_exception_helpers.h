#pragma once

#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

#include "gtest/gtest.h"

#include "pe_bliss2/exceptions/arm_common/arm_common_unwind_info.h"

template<typename Codes, typename Expected>
void test_created_unwind_code(std::uint8_t first_byte)
{
	std::vector<Codes> vec;
	ASSERT_NO_THROW(pe_bliss::exceptions::arm_common::create_unwind_code(
		std::byte{ first_byte }, vec));
	ASSERT_EQ(vec.size(), 1u);
	ASSERT_TRUE(std::get_if<Expected>(&vec.back()));
}
