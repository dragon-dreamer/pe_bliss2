#include <cstdint>
#include <limits>
#include <system_error>
#include <type_traits>

#include "gtest/gtest.h"

#include "utilities/safe_uint.h"

namespace
{
template<typename T>
class SafeUintTests : public testing::Test
{
public:
	template<typename... Value>
	[[nodiscard]] auto create(Value... value) noexcept
	{
		return utilities::safe_uint<T>(value...);
	}

	static constexpr const auto max_value = (std::numeric_limits<T>::max)();
};
} //namespace

using tested_types = ::testing::Types<std::uint8_t,
	std::uint16_t, std::uint32_t, std::uint64_t>;

TYPED_TEST_SUITE(SafeUintTests, tested_types);

TYPED_TEST(SafeUintTests, AssignTests)
{
	auto value = this->create();
	EXPECT_EQ(value.value(), 0u);

	value = this->create(1u);
	EXPECT_EQ(value.value(), 1u);

	value = 2u;
	EXPECT_EQ(value.value(), 2u);

	auto copy = this->create(value);
	EXPECT_EQ(copy.value(), 2u);

	copy = this->create(1u);
	EXPECT_EQ(copy.value(), 1u);
}

TYPED_TEST(SafeUintTests, CompareTests)
{
	auto value1 = this->create(1u);
	auto value2 = this->create(2u);
	EXPECT_EQ(value1, value1);
	EXPECT_LT(value1, value2);
	EXPECT_GT(value2, value1);
	EXPECT_LE(value1, value2);
	EXPECT_GE(value2, value1);
	EXPECT_TRUE(value1);
	EXPECT_FALSE(this->create());
}

TYPED_TEST(SafeUintTests, AddTests)
{
	auto value = this->create(1u);
	value += 2u;
	ASSERT_EQ(value.value(), 3u);

	auto copy = value + 2u;
	ASSERT_EQ(copy.value(), 5u);

	copy = copy + copy;
	ASSERT_EQ(copy.value(), 10u);

	EXPECT_THROW((void)(copy + this->max_value), std::system_error);

	ASSERT_EQ((this->create() + this->max_value).value(),
		this->max_value);

	value = 1u;
	ASSERT_EQ((value += value).value(), 2u);
	ASSERT_EQ(value.value(), 2u);
	
	ASSERT_EQ((value += 3u).value(), 5u);
	ASSERT_EQ(value.value(), 5u);
}

TYPED_TEST(SafeUintTests, SubTests)
{
	auto value = this->create(5u);
	value -= 2u;
	ASSERT_EQ(value.value(), 3u);

	auto copy = value - 2u;
	ASSERT_EQ(copy.value(), 1u);

	copy = copy - copy;
	ASSERT_EQ(copy.value(), 0u);

	EXPECT_THROW((void)(copy - 1u), std::system_error);
	EXPECT_THROW((void)(value - this->max_value), std::system_error);

	value = this->max_value;
	ASSERT_EQ((value - this->max_value).value(), 0u);

	ASSERT_EQ((value -= value).value(), 0u);
	ASSERT_EQ(value.value(), 0u);

	value = 10u;
	ASSERT_EQ((value -= 3u).value(), 7u);
	ASSERT_EQ(value.value(), 7u);
}

TYPED_TEST(SafeUintTests, AlignTests)
{
	auto value = this->create(1u);
	value.align_up(2u);
	ASSERT_EQ(value.value(), 2u);

	value = 3u;
	value.align_up(16u);
	ASSERT_EQ(value.value(), 16u);

	value.align_up(8u);
	ASSERT_EQ(value.value(), 16u);

	value = this->max_value;
	EXPECT_THROW(value.align_up(2ull), std::system_error);
	EXPECT_EQ(value.value(), this->max_value);
}

TEST(SafeUintTests, AddDifferentTypesTest)
{
	utilities::safe_uint<std::uint8_t> a = 1u;
	utilities::safe_uint<std::uint16_t> b = 2u;
	static constexpr auto max_value = (std::numeric_limits<std::uint8_t>::max)();
	utilities::safe_uint<std::uint16_t> max_value_minus_1 = max_value - 1u;

	auto sum1 = a + b;
	EXPECT_TRUE((std::is_same_v<decltype(sum1)::value_type, std::uint8_t>));
	ASSERT_EQ(sum1.value(), 3u);

	auto sum2 = b + a;
	EXPECT_TRUE((std::is_same_v<decltype(sum2)::value_type, std::uint16_t>));
	ASSERT_EQ(sum2.value(), 3u);

	ASSERT_EQ((a += max_value_minus_1).value(), max_value);
	ASSERT_THROW((a += b), std::system_error);
	ASSERT_EQ(a.value(), max_value);
	ASSERT_EQ(b.value(), 2u);
}

TEST(SafeUintTests, SubDifferentTypesTest)
{
	utilities::safe_uint<std::uint8_t> a = 1u;
	utilities::safe_uint<std::uint16_t> b = 2u;

	auto diff1 = b - a;
	EXPECT_TRUE((std::is_same_v<decltype(diff1)::value_type, std::uint16_t>));
	ASSERT_EQ(diff1.value(), 1u);

	auto sum2 = b + a;
	EXPECT_THROW((void)(a - b), std::system_error);

	ASSERT_EQ((b -= a).value(), 1u);
	ASSERT_EQ(b.value(), 1u);
	ASSERT_EQ(a.value(), 1u);

	b = 2u;
	ASSERT_THROW((a -= b), std::system_error);
	ASSERT_EQ(a.value(), 1u);
	ASSERT_EQ(b.value(), 2u);
}
