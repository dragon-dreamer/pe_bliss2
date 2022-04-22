#include <cstddef>
#include <cstdint>
#include <limits>

#include "gtest/gtest.h"

#include "utilities/math.h"

using namespace utilities;

TEST(MathTests, IsAlignedTest)
{
	EXPECT_TRUE(math::is_aligned<std::uint8_t>(1));
	EXPECT_TRUE(math::is_aligned<std::uint8_t>(2));
	EXPECT_TRUE(math::is_aligned<std::uint8_t>(3));

	EXPECT_FALSE(math::is_aligned<std::uint16_t>(1));
	EXPECT_TRUE(math::is_aligned<std::uint16_t>(2));
	EXPECT_FALSE(math::is_aligned<std::uint16_t>(3));
	EXPECT_TRUE(math::is_aligned<std::uint16_t>(4));

	EXPECT_FALSE(math::is_aligned<std::uint32_t>(1));
	EXPECT_FALSE(math::is_aligned<std::uint32_t>(2));
	EXPECT_FALSE(math::is_aligned<std::uint32_t>(3));
	EXPECT_TRUE(math::is_aligned<std::uint32_t>(4));
	EXPECT_TRUE(math::is_aligned<std::uint32_t>(8));

	EXPECT_FALSE(math::is_aligned<std::uint64_t>(1));
	EXPECT_FALSE(math::is_aligned<std::uint64_t>(2));
	EXPECT_FALSE(math::is_aligned<std::uint64_t>(3));
	EXPECT_FALSE(math::is_aligned<std::uint64_t>(4));
	EXPECT_TRUE(math::is_aligned<std::uint64_t>(8));
}

TEST(MathTests, AlignDownTest)
{
	EXPECT_EQ(math::align_down(1u, 1u), 1u);
	EXPECT_EQ(math::align_down(2u, 1u), 2u);
	EXPECT_EQ(math::align_down(3u, 1u), 3u);

	EXPECT_EQ(math::align_down(1u, 2u), 0u);
	EXPECT_EQ(math::align_down(2u, 2u), 2u);
	EXPECT_EQ(math::align_down(3u, 2u), 2u);
	EXPECT_EQ(math::align_down(4u, 2u), 4u);

	EXPECT_EQ(math::align_down(1u, 4u), 0u);
	EXPECT_EQ(math::align_down(2u, 4u), 0u);
	EXPECT_EQ(math::align_down(3u, 4u), 0u);
	EXPECT_EQ(math::align_down(4u, 4u), 4u);
	EXPECT_EQ(math::align_down(5u, 4u), 4u);
	EXPECT_EQ(math::align_down(8u, 4u), 8u);
}

TEST(MathTests, AlignUpTest)
{
	EXPECT_EQ(math::align_up(0u, 1u), 0u);
	EXPECT_EQ(math::align_up(1u, 1u), 1u);
	EXPECT_EQ(math::align_up(2u, 1u), 2u);
	EXPECT_EQ(math::align_up(3u, 1u), 3u);

	EXPECT_EQ(math::align_up(0u, 2u), 0u);
	EXPECT_EQ(math::align_up(1u, 2u), 2u);
	EXPECT_EQ(math::align_up(2u, 2u), 2u);
	EXPECT_EQ(math::align_up(3u, 2u), 4u);
	EXPECT_EQ(math::align_up(4u, 2u), 4u);

	EXPECT_EQ(math::align_up(1u, 4u), 4u);
	EXPECT_EQ(math::align_up(2u, 4u), 4u);
	EXPECT_EQ(math::align_up(3u, 4u), 4u);
	EXPECT_EQ(math::align_up(4u, 4u), 4u);
	EXPECT_EQ(math::align_up(5u, 4u), 8u);
	EXPECT_EQ(math::align_up(8u, 4u), 8u);
}

namespace
{
template<typename T>
void test_is_sum_safe()
{
	static constexpr T max_val = (std::numeric_limits<T>::max)();
	static constexpr T val_0 = 0;
	static constexpr T val_1 = 1;
	EXPECT_TRUE(math::is_sum_safe(val_1, val_1));
	EXPECT_TRUE(math::is_sum_safe(max_val, val_0));
	EXPECT_TRUE(math::is_sum_safe(val_0, max_val));
	EXPECT_FALSE(math::is_sum_safe(max_val, val_1));
	EXPECT_FALSE(math::is_sum_safe(val_1, max_val));
}

template<typename T>
void test_add_if_sum_safe()
{
	static constexpr T max_val = (std::numeric_limits<T>::max)();
	static constexpr T val_0 = 0;
	static constexpr T val_1 = 1;
	T target_1 = val_1;
	T target_max = max_val;

	EXPECT_TRUE(math::add_if_safe(target_1, val_1));
	EXPECT_EQ(target_1, 2u);

	EXPECT_TRUE(math::add_if_safe(target_1, val_0));
	EXPECT_EQ(target_1, 2u);

	EXPECT_FALSE(math::add_if_safe(target_1, max_val));
	EXPECT_EQ(target_1, 2u);

	EXPECT_FALSE(math::add_if_safe(target_max, val_1));
	EXPECT_EQ(target_max, max_val);
}

template<typename T>
void test_align_up_if_safe()
{
	T target = 3u;
	EXPECT_TRUE(math::align_up_if_safe(target, 1u));
	EXPECT_EQ(target, 3u);
	EXPECT_TRUE(math::align_up_if_safe(target, 2u));
	EXPECT_EQ(target, 4u);
	EXPECT_TRUE(math::align_up_if_safe(target, 4u));
	EXPECT_EQ(target, 4u);

	target = (std::numeric_limits<T>::max)();
	EXPECT_FALSE(math::align_up_if_safe(target, 2u));
}
} //namespace

TEST(MathTests, IsSumSafeTestU8)
{
	test_is_sum_safe<std::uint8_t>();
}

TEST(MathTests, IsSumSafeTestU16)
{
	test_is_sum_safe<std::uint16_t>();
}

TEST(MathTests, IsSumSafeTestU32)
{
	test_is_sum_safe<std::uint32_t>();
}

TEST(MathTests, IsSumSafeTestU64)
{
	test_is_sum_safe<std::uint64_t>();
}

TEST(MathTests, AddIfSafeTestU8)
{
	test_add_if_sum_safe<std::uint8_t>();
}

TEST(MathTests, AddIfSafeTestU16)
{
	test_add_if_sum_safe<std::uint16_t>();
}

TEST(MathTests, AddIfSafeTestU32)
{
	test_add_if_sum_safe<std::uint32_t>();
}

TEST(MathTests, AddIfSafeTestU64)
{
	test_add_if_sum_safe<std::uint64_t>();
}

TEST(MathTests, AlignUpIfSafeTestU8)
{
	test_align_up_if_safe<std::uint8_t>();
}

TEST(MathTests, AlignUpIfSafeTestU16)
{
	test_align_up_if_safe<std::uint16_t>();
}

TEST(MathTests, AlignUpIfSafeTestU32)
{
	test_align_up_if_safe<std::uint32_t>();
}

TEST(MathTests, AlignUpIfSafeTestU64)
{
	test_align_up_if_safe<std::uint64_t>();
}

TEST(MathTests, AddOffsetIfSafeTest)
{
	std::size_t pos = 0;
	EXPECT_TRUE(math::add_offset_if_safe(pos, 10u));
	EXPECT_EQ(pos, 10u);

	EXPECT_TRUE(math::add_offset_if_safe(pos, -8));
	EXPECT_EQ(pos, 2u);

	EXPECT_FALSE(math::add_offset_if_safe(pos, -3));
	EXPECT_EQ(pos, 2u);

	EXPECT_TRUE(math::add_offset_if_safe(pos, -2));
	EXPECT_EQ(pos, 0u);

	constexpr auto max_val = (std::numeric_limits<std::size_t>::max)();
	pos = max_val;
	EXPECT_FALSE(math::add_offset_if_safe(pos, 1u));
	EXPECT_EQ(pos, max_val);

	EXPECT_TRUE(math::add_offset_if_safe(pos, 0u));
	EXPECT_EQ(pos, max_val);
}
