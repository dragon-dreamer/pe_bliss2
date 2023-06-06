#include "gtest/gtest.h"

#include <cstddef>
#include <memory>

#include "buffers/input_container_buffer.h"
#include "buffers/input_buffer_stateful_wrapper.h"

#include "pe_bliss2/core/overlay.h"

#include "tests/pe_bliss2/pe_error_helper.h"

TEST(OverlayTests, DeserializeTestEmpty)
{
	auto buf = std::make_shared<buffers::input_container_buffer>();
	buffers::input_buffer_stateful_wrapper wrapper(buf);

	pe_bliss::core::overlay overlay;
	EXPECT_NO_THROW(overlay.deserialize(100u, 100u, 1u, wrapper, false));
	EXPECT_EQ(overlay.size(), 0u);
}

TEST(OverlayTests, DeserializeTest)
{
	auto buf = std::make_shared<buffers::input_container_buffer>();
	buffers::input_buffer_stateful_wrapper wrapper(buf);
	buf->get_container() = {
		std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}
	};

	pe_bliss::core::overlay overlay;
	EXPECT_NO_THROW(overlay.deserialize(3u, 2u, 1u, wrapper, true));
	EXPECT_TRUE(overlay.is_copied());
	ASSERT_EQ(overlay.copied_data().size(), 1u);
	EXPECT_EQ(overlay.copied_data()[0], std::byte{ 5 });

	EXPECT_NO_THROW(overlay.deserialize(3u, 2u, 1u, wrapper, false));
	EXPECT_FALSE(overlay.is_copied());
	ASSERT_EQ(overlay.copied_data().size(), 1u);
	EXPECT_EQ(overlay.copied_data()[0], std::byte{ 5 });
}
