#include <span>
#include <vector>
#include <unordered_map>

#include "gtest/gtest.h"

#include "utilities/range_helpers.h"

TEST(RangeHelpersTests, HashMapTest)
{
	std::unordered_map<std::vector<int>, int,
		utilities::range_hash, utilities::range_equal> map;

	ASSERT_TRUE((map.emplace(std::vector{1, 2, 3}, 0).second));
	ASSERT_TRUE((map.emplace(std::vector{4, 5, 6, 7}, 1).second));
	ASSERT_FALSE((map.emplace(std::vector{1, 2, 3}, 2).second));

	std::vector target{1, 2, 3};
	auto it = map.find(std::span(target));
	ASSERT_NE(it, map.end());
	EXPECT_EQ(it->second, 0);
}
