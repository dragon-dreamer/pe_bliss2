#include <cstddef>
#include <memory>
#include <ranges>
#include <system_error>
#include <vector>
#include <utility>

#include "gtest/gtest.h"

#include "buffers/output_memory_buffer.h"
#include "buffers/ref_buffer.h"
#include "tests/tests/buffers/buffer_helpers.h"

namespace
{
void check_buffer_empty(const buffers::ref_buffer& buf)
{
	EXPECT_TRUE(buf.empty());
	EXPECT_EQ(buf.size(), 0u);
	EXPECT_EQ(buf.data()->size(), 0u);
	EXPECT_EQ(buf.copied_data().size(), 0u);

	std::vector<std::byte> data;
	buffers::output_memory_buffer out_buf(data);
	EXPECT_EQ(buf.serialize(out_buf, 0u), 0u);
	EXPECT_TRUE(data.empty());
}

void test_serialize(const std::vector<std::byte>& src,
	const buffers::ref_buffer& buf,
	std::size_t pos, std::size_t length)
{
	std::vector<std::byte> data;
	buffers::output_memory_buffer out_buf(data);
	EXPECT_EQ(buf.serialize(out_buf, pos, length), length);
	EXPECT_EQ(data.size(), length);
	EXPECT_TRUE(std::ranges::equal(src | std::views::drop(pos)
		| std::views::take(length), data));
}

void move_tests(
	const std::shared_ptr<buffers::input_container_buffer>& input_buf,
	buffers::ref_buffer& buf, bool is_buffer_copied)
{
	buffers::ref_buffer moved_buf;
	moved_buf = std::move(buf);
	EXPECT_EQ(input_buf.use_count(), is_buffer_copied ? 1u : 2u);
	EXPECT_TRUE(!moved_buf.empty());
	EXPECT_EQ(moved_buf.size(), input_buf->size());
	EXPECT_EQ(moved_buf.data()->size(), input_buf->size());

	auto moved_buf2(std::move(moved_buf));
	EXPECT_EQ(input_buf.use_count(), is_buffer_copied ? 1u : 2u);
	EXPECT_TRUE(!moved_buf2.empty());
	EXPECT_EQ(moved_buf2.size(), input_buf->size());
	EXPECT_EQ(moved_buf2.data()->size(), input_buf->size());

	std::vector<std::byte> data;
	buffers::output_memory_buffer out_buf(data);
	EXPECT_NO_THROW(moved_buf2.serialize(out_buf));
	EXPECT_EQ(data, input_buf->get_container());
}

void copy_tests(
	const std::shared_ptr<buffers::input_container_buffer>& input_buf,
	buffers::ref_buffer& buf, bool is_buffer_copied)
{
	buffers::ref_buffer copied_buf;
	copied_buf = buf;
	EXPECT_EQ(input_buf.use_count(), is_buffer_copied ? 1u : 3u);
	EXPECT_TRUE(!copied_buf.empty());
	EXPECT_EQ(copied_buf.size(), input_buf->size());
	EXPECT_EQ(copied_buf.data()->size(), input_buf->size());
	EXPECT_TRUE(!buf.empty());
	EXPECT_EQ(buf.size(), input_buf->size());
	EXPECT_EQ(buf.data()->size(), input_buf->size());

	auto copied_buf2(copied_buf);
	EXPECT_EQ(input_buf.use_count(), is_buffer_copied ? 1u : 4u);
	EXPECT_TRUE(!copied_buf2.empty());
	EXPECT_EQ(copied_buf2.size(), input_buf->size());
	EXPECT_EQ(copied_buf2.data()->size(), input_buf->size());
	EXPECT_TRUE(!copied_buf.empty());
	EXPECT_EQ(copied_buf.size(), input_buf->size());
	EXPECT_EQ(copied_buf.data()->size(), input_buf->size());

	std::vector<std::byte> data;
	buffers::output_memory_buffer out_buf(data);
	EXPECT_NO_THROW(copied_buf.serialize(out_buf));
	EXPECT_EQ(data, input_buf->get_container());

	data.clear();
	out_buf.set_wpos(0);
	EXPECT_NO_THROW(buf.serialize(out_buf));
	EXPECT_EQ(data, input_buf->get_container());

	data.clear();
	out_buf.set_wpos(0);
	EXPECT_NO_THROW(copied_buf2.serialize(out_buf));
	EXPECT_EQ(data, input_buf->get_container());
}

class RefBufferTestsFixture : public ::testing::TestWithParam<bool> {
};
} //namespace

TEST(BufferTests, EmptyRefBufferTest)
{
	buffers::ref_buffer buf;
	check_buffer_empty(buf);

	EXPECT_NO_THROW(buf.copy_referenced_buffer());
	check_buffer_empty(buf);
}

TEST(BufferTests, CopiedRefBufferTest)
{
	auto input_buf = create_input_container_buffer(100u);
	buffers::ref_buffer buf;

	for (std::size_t i = 0; i != 2; ++i) //loop to test ref_buffer reuse
	{
		EXPECT_NO_THROW(buf.deserialize(input_buf, true));
		EXPECT_EQ(input_buf.use_count(), 1u);

		EXPECT_TRUE(!buf.empty());
		EXPECT_EQ(buf.size(), input_buf->size());
		EXPECT_EQ(buf.data()->size(), input_buf->size());
		EXPECT_EQ(std::as_const(buf).copied_data().size(), input_buf->size());
		EXPECT_EQ(input_buf->get_container(), buf.copied_data());

		test_serialize(input_buf->get_container(), buf, 0u, 10u);
		test_serialize(input_buf->get_container(), buf, 1u, 20u);

		std::vector<std::byte> data;
		buffers::output_memory_buffer out_buf(data);
		EXPECT_THROW(buf.serialize(out_buf, 1u, input_buf->size() + 10u),
			std::system_error);
	}
}

TEST(BufferTests, ReferencedRefBufferTest)
{
	auto input_buf = create_input_container_buffer(100u);
	buffers::ref_buffer buf;

	for (std::size_t i = 0; i != 2; ++i) //loop to test ref_buffer reuse
	{
		EXPECT_NO_THROW(buf.deserialize(input_buf, false));
		EXPECT_EQ(input_buf.use_count(), 2u);

		EXPECT_TRUE(!buf.empty());
		EXPECT_EQ(buf.size(), input_buf->size());
		EXPECT_EQ(buf.data()->size(), input_buf->size());

		test_serialize(input_buf->get_container(), buf, 0u, 10u);
		test_serialize(input_buf->get_container(), buf, 1u, 20u);
		EXPECT_EQ(input_buf.use_count(), 2u);
		EXPECT_THROW((void)std::as_const(buf).copied_data(),
			std::system_error);

		std::vector<std::byte> data;
		buffers::output_memory_buffer out_buf(data);
		EXPECT_THROW(buf.serialize(out_buf, 1u, input_buf->size() + 10u), std::system_error);

		EXPECT_EQ(buf.copied_data().size(), input_buf->size());
		EXPECT_EQ(input_buf->get_container(), buf.copied_data());
	}
}

TEST_P(RefBufferTestsFixture, MoveRefBufferTest)
{
	auto input_buf = create_input_container_buffer(100u);
	buffers::ref_buffer buf;
	EXPECT_NO_THROW(buf.deserialize(input_buf, GetParam()));
	move_tests(input_buf, buf, GetParam());
}

INSTANTIATE_TEST_SUITE_P(
	MoveReferencedRefBufferTest,
	RefBufferTestsFixture,
	::testing::Values(true, false));

TEST_P(RefBufferTestsFixture, CopyRefBufferTest)
{
	auto input_buf = create_input_container_buffer(100u);
	buffers::ref_buffer buf;
	EXPECT_NO_THROW(buf.deserialize(input_buf, GetParam()));
	copy_tests(input_buf, buf, GetParam());
}

INSTANTIATE_TEST_SUITE_P(
	CopyReferencedRefBufferTest,
	RefBufferTestsFixture,
	::testing::Values(true, false));
