#include "gtest/gtest.h"

#include <cstddef>
#include <memory>

#include "buffers/input_container_buffer.h"

#include "pe_bliss2/core/overlay.h"

#include "tests/tests/pe_bliss2/pe_error_helper.h"

TEST(OverlayTests, DeserializeTestEmpty)
{
	auto buf = std::make_shared<buffers::input_container_buffer>();

	pe_bliss::core::overlay overlay;
	EXPECT_NO_THROW(overlay.deserialize(100u, 100u, 1u, buf, false));
	EXPECT_TRUE(overlay.empty());
}

TEST(OverlayTests, DeserializeTest)
{
	auto buf = std::make_shared<buffers::input_container_buffer>();
	buf->get_container() = {
		std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}
	};

	pe_bliss::core::overlay overlay;
	EXPECT_NO_THROW(overlay.deserialize(3u, 2u, 1u, buf, true));
	EXPECT_FALSE(overlay.empty());
	EXPECT_TRUE(overlay.is_copied());
	ASSERT_EQ(overlay.copied_data().size(), 1u);
	EXPECT_EQ(overlay.copied_data()[0], std::byte{ 5 });

	EXPECT_NO_THROW(overlay.deserialize(3u, 2u, 1u, buf, false));
	EXPECT_FALSE(overlay.empty());
	EXPECT_FALSE(overlay.is_copied());
	ASSERT_EQ(overlay.copied_data().size(), 1u);
	EXPECT_EQ(overlay.copied_data()[0], std::byte{ 5 });
}
