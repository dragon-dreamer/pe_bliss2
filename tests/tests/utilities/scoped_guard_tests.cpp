#include <stdexcept>
#include <type_traits>

#include "gtest/gtest.h"

#include "utilities/scoped_guard.h"

TEST(ScopedGuardTests, ScopedGuardTest)
{
	int value = 0;
	auto lambda = [&value] () noexcept { value = 1; };

	{
		utilities::scoped_guard g(lambda);
		EXPECT_EQ(value, 0);
	}

	EXPECT_EQ(value, 1);

	using scopeg_guard_type = utilities::scoped_guard<decltype(lambda)>;
	EXPECT_TRUE(std::is_nothrow_destructible_v<scopeg_guard_type>);
	EXPECT_FALSE(std::is_copy_assignable_v<scopeg_guard_type>);
	EXPECT_FALSE(std::is_copy_constructible_v<scopeg_guard_type>);
	EXPECT_FALSE(std::is_move_assignable_v<scopeg_guard_type>);
	EXPECT_FALSE(std::is_move_constructible_v<scopeg_guard_type>);
	EXPECT_FALSE(std::is_constructible_v<scopeg_guard_type>);
}

namespace
{
void guard_throw()
{
	auto lambda = [] { throw std::runtime_error(""); };
	utilities::scoped_guard g(lambda);
	EXPECT_FALSE(std::is_nothrow_destructible_v<decltype(g)>);
}
} //namespace

TEST(ScopedGuardTests, ScopedGuardThrowTest)
{
	auto lambda = [] { throw std::runtime_error(""); };

	EXPECT_THROW(guard_throw(), std::runtime_error);
}
