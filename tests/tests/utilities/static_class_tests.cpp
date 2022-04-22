#include <type_traits>

#include "gtest/gtest.h"

#include "utilities/static_class.h"

TEST(StaticClassTests, StaticClassTest)
{
	using namespace utilities;
	EXPECT_FALSE(std::is_copy_assignable_v<static_class>);
	EXPECT_FALSE(std::is_copy_constructible_v<static_class>);
	EXPECT_FALSE(std::is_move_assignable_v<static_class>);
	EXPECT_FALSE(std::is_move_constructible_v<static_class>);
	EXPECT_FALSE(std::is_constructible_v<static_class>);
}
