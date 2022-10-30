#include "utilities/shannon_entropy.h"

#include <cstddef>

#include "gtest/gtest.h"

TEST(ShannonEntropyTest, Smoke)
{
	EXPECT_EQ(utilities::shannon_entropy::maximum_entropy, 8u);

	utilities::shannon_entropy entropy;
	EXPECT_EQ(entropy.finalize(), 0.f);

	for (std::size_t i = 0; i != 256; ++i)
		entropy.update(static_cast<std::byte>(i));

	EXPECT_NEAR(entropy.finalize(), 8.f, 0.01f);
}
