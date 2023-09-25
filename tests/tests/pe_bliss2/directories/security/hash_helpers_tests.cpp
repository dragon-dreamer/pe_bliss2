#include "pe_bliss2/security/hash_helpers.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "buffers/input_container_buffer.h"

#include "cryptopp/cryptlib.h"

#include "pe_bliss2/detail/packed_serialization.h"
#include "pe_bliss2/pe_error.h"

using namespace pe_bliss::security;

namespace
{
class hash_transformation_mock : public CryptoPP::HashTransformation
{
public:
	MOCK_METHOD(void, Update, (const CryptoPP::byte*, std::size_t), (override));
	MOCK_METHOD(unsigned int, DigestSize, (), (const, override));
	MOCK_METHOD(void, TruncatedFinal, (CryptoPP::byte*, std::size_t), (override));
};

class non_contiguous_buffer final : public buffers::input_buffer_interface
{
public:
	using container_type = buffers::input_container_buffer::container_type;

public:
	non_contiguous_buffer(std::size_t absolute_offset = 0,
		std::size_t relative_offset = 0)
		: container_(absolute_offset, relative_offset)
	{
	}

	virtual std::size_t size() override
	{
		return container_.size();
	}

	virtual std::size_t read(std::size_t pos,
		std::size_t count, std::byte* data) override
	{
		return container_.read(pos, count, data);
	}

	[[nodiscard]]
	container_type& get_container() noexcept
	{
		return container_.get_container();
	}

	[[nodiscard]]
	const container_type& get_container() const noexcept
	{
		return container_.get_container();
	}

private:
	buffers::input_container_buffer container_;
};
} //namespace

TEST(HashHelperTests, HashTransformContiguousBuffer)
{
	buffers::input_container_buffer buf;
	buf.get_container() = { std::byte{1}, std::byte{0xffu}, std::byte{2} };
	buf.get_container().resize(4096);
	::testing::StrictMock<hash_transformation_mock> transform;
	EXPECT_CALL(transform, Update(reinterpret_cast<const CryptoPP::byte*>(
		buf.get_container().data() + 1), buf.get_container().size() - 3)).Times(1);
	update_hash(buf, 1, buf.get_container().size() - 2, transform);
}

TEST(HashHelperTests, HashTransformInvalidBounds)
{
	buffers::input_container_buffer buf;
	::testing::StrictMock<hash_transformation_mock> transform;
	EXPECT_THROW(update_hash(buf, 5, 1, transform), pe_bliss::pe_error);
}

TEST(HashHelperTests, HashTransformNonContiguousBuffer)
{
	non_contiguous_buffer buf;
	buf.get_container() = { std::byte{1}, std::byte{0xffu}, std::byte{2} };
	buf.get_container().resize(4096);
	buf.get_container().back() = std::byte{ 5 };

	std::vector<std::byte> copy;
	copy.emplace_back(std::byte{1});
	::testing::StrictMock<hash_transformation_mock> transform;
	EXPECT_CALL(transform, Update(::testing::_, ::testing::_))
		.Times(::testing::AtLeast(1))
		.WillRepeatedly(
		[&copy](const CryptoPP::byte* data, std::size_t size) {
			const auto* bytes = reinterpret_cast<const std::byte*>(data);
			copy.insert(copy.end(), bytes, bytes + size);
		});

	update_hash(buf, 1, buf.get_container().size(), transform);
	EXPECT_EQ(buf.get_container(), copy);
}

namespace
{
class PageHashHelperTests : public ::testing::Test
{
public:
	static constexpr int hash_size = 32;
	static constexpr std::uint32_t page_size = 1024;
	static constexpr std::uint32_t page1_offset = 12345;
	static constexpr std::uint32_t page2_offset = 56789;
	static constexpr std::uint8_t hash_filler1 = 0x12u;
	static constexpr std::uint8_t hash_filler2 = 0x34u;
	static constexpr std::uint8_t hash_filler3 = 0x56u;
	static constexpr std::uint32_t skipped_bytes1 = 10;
	static constexpr std::uint32_t skipped_bytes2 = 15;

	PageHashHelperTests()
	{
		EXPECT_CALL(transform, DigestSize())
			.Times(::testing::AtLeast(0))
			.WillRepeatedly(::testing::Return(hash_size));
	}

	static void check_page1(const std::vector<std::byte>& hashes)
	{
		std::array<std::byte, 4u> page1_offset_serialized{};
		pe_bliss::detail::packed_serialization<>::serialize(page1_offset,
			page1_offset_serialized.data());
		EXPECT_TRUE(std::ranges::equal(std::span(hashes).subspan(
			0u, sizeof(std::uint32_t)), //page 1 offset
			page1_offset_serialized));
		EXPECT_TRUE(std::ranges::all_of(std::span(hashes).subspan(
			sizeof(std::uint32_t), hash_size), //page 1 hash
			[](auto b) { return b == std::byte{ hash_filler1 }; }));
	}

	static void check_page2(const std::vector<std::byte>& hashes,
		std::uint32_t remaining_page1_bytes)
	{
		std::array<std::byte, 4u> page2_offset_serialized{};
		pe_bliss::detail::packed_serialization<>::serialize(
			page2_offset + remaining_page1_bytes, //offset + remaining bytes from the
			//first unfinished page
			page2_offset_serialized.data());
		EXPECT_TRUE(std::ranges::equal(std::span(hashes).subspan(
			sizeof(std::uint32_t) + hash_size, sizeof(std::uint32_t)), //page 2 offset
			page2_offset_serialized));
		EXPECT_TRUE(std::ranges::all_of(std::span(hashes).subspan(
			2 * sizeof(std::uint32_t) + hash_size, hash_size), //page 2 hash
			[](auto b) { return b == std::byte{ hash_filler2 }; }));
	}

	static void check_page3(const std::vector<std::byte>& hashes,
		std::uint32_t remaining_page12_bytes)
	{
		std::array<std::byte, 4u> page3_offset_serialized{};
		pe_bliss::detail::packed_serialization<>::serialize(
			page2_offset + remaining_page12_bytes,
			page3_offset_serialized.data());
		EXPECT_TRUE(std::ranges::equal(std::span(hashes).subspan(
			2 * (sizeof(std::uint32_t) + hash_size), sizeof(std::uint32_t)), //page 3 offset
			page3_offset_serialized));
		EXPECT_TRUE(std::ranges::all_of(std::span(hashes).subspan(
			2 * (sizeof(std::uint32_t) + hash_size) + sizeof(std::uint32_t), hash_size), //page 3 hash
			[](auto b) { return b == std::byte{ hash_filler3 }; }));
	}

	static void check_page4(const std::vector<std::byte>& hashes,
		std::uint32_t last_offset)
	{
		std::array<std::byte, 4u> page4_offset_serialized{};
		//page 4 offset: last offset passed to update()
		pe_bliss::detail::packed_serialization<>::serialize(
			page2_offset + last_offset,
			page4_offset_serialized.data());

		EXPECT_TRUE(std::ranges::equal(std::span(hashes).subspan(
			3 * (sizeof(std::uint32_t) + hash_size), sizeof(std::uint32_t)), //page 4 offset
			page4_offset_serialized));
		EXPECT_TRUE(std::ranges::all_of(std::span(hashes).subspan(
			3 * (sizeof(std::uint32_t) + hash_size) + sizeof(std::uint32_t), hash_size), //final page 4 (zero)
			[](auto b) { return b == std::byte{}; }));
	}

	::testing::StrictMock<hash_transformation_mock> transform;
	page_hash_state state{ transform, page_size };
	static constexpr std::array<std::byte, 1> dummy_data{};
};
} //namespace

TEST_F(PageHashHelperTests, PageHashStateEmpty)
{
	auto hashes = std::move(state).get_page_hashes();
	//check last empty page
	EXPECT_EQ(hashes.size(), hash_size + sizeof(std::uint32_t));
	EXPECT_TRUE(std::ranges::all_of(hashes, [](auto b) { return b == std::byte{}; }));
}

TEST_F(PageHashHelperTests, PageHashStateNoSkipped)
{
	EXPECT_CALL(transform, Update(
		reinterpret_cast<const CryptoPP::byte*>(dummy_data.data()), page_size / 2))
		.Times(1);
	state.update(dummy_data.data(), page1_offset, page_size / 2);

	::testing::InSequence seq;
	//remaining bytes of the first unfinished page...
	EXPECT_CALL(transform, Update(
		reinterpret_cast<const CryptoPP::byte*>(dummy_data.data()), page_size / 2))
		.Times(1);
	EXPECT_CALL(transform, TruncatedFinal(::testing::_, hash_size))
		.WillOnce([](CryptoPP::byte* data, std::size_t size) {
			std::memset(data, hash_filler1, size);
		});
	//full next page
	EXPECT_CALL(transform, Update(
		reinterpret_cast<const CryptoPP::byte*>(
			dummy_data.data() + page_size / 2), page_size))
		.Times(1);
	EXPECT_CALL(transform, TruncatedFinal(::testing::_, hash_size))
		.WillOnce([](CryptoPP::byte* data, std::size_t size) {
			std::memset(data, hash_filler2, size);
		});
	//half of page remaining
	EXPECT_CALL(transform, Update(
		reinterpret_cast<const CryptoPP::byte*>(
			dummy_data.data() + page_size + page_size / 2), page_size / 2))
		.Times(1);
	state.update(dummy_data.data(), page2_offset, page_size * 2);

	//half of page unfilled and now being filled with zeros
	EXPECT_CALL(transform, Update(::testing::_, page_size / 2))
		.WillOnce([](const CryptoPP::byte* data, std::size_t size) {
			while (size--)
			{
				ASSERT_EQ(data[size], 0);
			}
		});
	EXPECT_CALL(transform, TruncatedFinal(::testing::_, hash_size))
		.WillOnce([](CryptoPP::byte* data, std::size_t size) {
			std::memset(data, hash_filler3, size);
		});

	auto hashes = std::move(state).get_page_hashes();
	EXPECT_EQ(hashes.size(), 4u * (hash_size + sizeof(std::uint32_t)));

	check_page1(hashes);
	check_page2(hashes, page_size / 2);
	check_page3(hashes, page_size / 2 + page_size);
	check_page4(hashes, page_size * 2);
}

TEST_F(PageHashHelperTests, PageHashStateEmptyNextPage)
{
	state.next_page(); //Does nothing

	state.add_skipped_bytes(skipped_bytes1);
	state.next_page(); //Does nothing
}

TEST_F(PageHashHelperTests, PageHashStateWithNextPage)
{
	::testing::InSequence seq;
	EXPECT_CALL(transform, Update(
		reinterpret_cast<const CryptoPP::byte*>(dummy_data.data()), page_size / 2))
		.Times(1);
	state.update(dummy_data.data(), page1_offset, page_size / 2);
	
	EXPECT_CALL(transform, Update(::testing::_, page_size / 2))
		.WillOnce([](const CryptoPP::byte* data, std::size_t size) {
		while (size--)
		{
			ASSERT_EQ(data[size], 0);
		}
	});
	EXPECT_CALL(transform, TruncatedFinal(::testing::_, hash_size))
		.WillOnce([](CryptoPP::byte* data, std::size_t size) {
		std::memset(data, hash_filler1, size);
	});
	state.next_page(); //Finalize first unfinished page

	//full next page
	EXPECT_CALL(transform, Update(
		reinterpret_cast<const CryptoPP::byte*>(
			dummy_data.data()), page_size))
		.Times(1);
	EXPECT_CALL(transform, TruncatedFinal(::testing::_, hash_size))
		.WillOnce([](CryptoPP::byte* data, std::size_t size) {
		std::memset(data, hash_filler2, size);
	});
	//full next page
	EXPECT_CALL(transform, Update(
		reinterpret_cast<const CryptoPP::byte*>(
			dummy_data.data() + page_size), page_size))
		.Times(1);
	EXPECT_CALL(transform, TruncatedFinal(::testing::_, hash_size))
		.WillOnce([](CryptoPP::byte* data, std::size_t size) {
		std::memset(data, hash_filler3, size);
	});
	state.update(dummy_data.data(), page2_offset, page_size * 2);

	state.next_page(); //Will do nothing, full page was written
	
	auto hashes = std::move(state).get_page_hashes();
	EXPECT_EQ(hashes.size(), 4u * (hash_size + sizeof(std::uint32_t)));

	check_page1(hashes);
	check_page2(hashes, 0u);
	check_page3(hashes, page_size);
	check_page4(hashes, page_size * 2);
}

TEST_F(PageHashHelperTests, PageHashStateWithSkipped)
{
	EXPECT_CALL(transform, Update(
		reinterpret_cast<const CryptoPP::byte*>(dummy_data.data()),
		page_size / 2 - skipped_bytes1)).Times(1);
	state.add_skipped_bytes(skipped_bytes1);
	state.update(dummy_data.data(), page1_offset + skipped_bytes1,
		page_size / 2 - skipped_bytes1);
	state.add_skipped_bytes(skipped_bytes2);

	::testing::InSequence seq;
	//remaining bytes of the first unfinished page...
	EXPECT_CALL(transform, Update(
		reinterpret_cast<const CryptoPP::byte*>(dummy_data.data()),
		page_size / 2 - skipped_bytes2)).Times(1);
	EXPECT_CALL(transform, TruncatedFinal(::testing::_, hash_size))
		.WillOnce([](CryptoPP::byte* data, std::size_t size) {
			std::memset(data, hash_filler1, size);
		});
	//full next page
	EXPECT_CALL(transform, Update(
		reinterpret_cast<const CryptoPP::byte*>(
			dummy_data.data() + page_size / 2 - skipped_bytes2), page_size))
		.Times(1);
	EXPECT_CALL(transform, TruncatedFinal(::testing::_, hash_size))
		.WillOnce([](CryptoPP::byte* data, std::size_t size) {
			std::memset(data, hash_filler2, size);
		});
	//half of page remaining
	EXPECT_CALL(transform, Update(
		reinterpret_cast<const CryptoPP::byte*>(
			dummy_data.data() + page_size + page_size / 2 - skipped_bytes2), page_size / 2))
		.Times(1);
	state.update(dummy_data.data(), page2_offset, page_size * 2 - skipped_bytes2);

	//half of page unfilled and now being filled with zeros
	EXPECT_CALL(transform, Update(::testing::_, page_size / 2))
		.WillOnce([](const CryptoPP::byte* data, std::size_t size) {
			while (size--)
			{
				ASSERT_EQ(data[size], 0);
			}
		});
	EXPECT_CALL(transform, TruncatedFinal(::testing::_, hash_size))
		.WillOnce([](CryptoPP::byte* data, std::size_t size) {
			std::memset(data, hash_filler3, size);
		});

	auto hashes = std::move(state).get_page_hashes();
	EXPECT_EQ(hashes.size(), 4u * (hash_size + sizeof(std::uint32_t)));

	check_page1(hashes);
	check_page2(hashes, page_size / 2 - skipped_bytes2);
	check_page3(hashes, page_size / 2 + page_size - skipped_bytes2);
	check_page4(hashes, page_size * 2 - skipped_bytes2);
}

TEST_F(PageHashHelperTests, PageHashStateWithNextPageAndSkipped)
{
	::testing::InSequence seq;
	EXPECT_CALL(transform, Update(
		reinterpret_cast<const CryptoPP::byte*>(dummy_data.data()),
		page_size / 2 - skipped_bytes1))
		.Times(1);
	state.add_skipped_bytes(skipped_bytes1);
	state.update(dummy_data.data(), page1_offset + skipped_bytes1,
		page_size / 2 - skipped_bytes1);
	state.add_skipped_bytes(skipped_bytes2);

	EXPECT_THROW(state.add_skipped_bytes(page_size), pe_bliss::pe_error);

	EXPECT_CALL(transform, Update(::testing::_, page_size / 2
		- skipped_bytes2))
		.WillOnce([](const CryptoPP::byte* data, std::size_t size) {
		while (size--)
		{
			ASSERT_EQ(data[size], 0);
		}
	});
	EXPECT_CALL(transform, TruncatedFinal(::testing::_, hash_size))
		.WillOnce([](CryptoPP::byte* data, std::size_t size) {
		std::memset(data, hash_filler1, size);
	});
	state.next_page(); //Finalize first unfinished page

	//full next page
	EXPECT_CALL(transform, Update(
		reinterpret_cast<const CryptoPP::byte*>(
			dummy_data.data()), page_size))
		.Times(1);
	EXPECT_CALL(transform, TruncatedFinal(::testing::_, hash_size))
		.WillOnce([](CryptoPP::byte* data, std::size_t size) {
		std::memset(data, hash_filler2, size);
	});
	//full next page
	EXPECT_CALL(transform, Update(
		reinterpret_cast<const CryptoPP::byte*>(
			dummy_data.data() + page_size), page_size))
		.Times(1);
	EXPECT_CALL(transform, TruncatedFinal(::testing::_, hash_size))
		.WillOnce([](CryptoPP::byte* data, std::size_t size) {
		std::memset(data, hash_filler3, size);
	});
	state.update(dummy_data.data(), page2_offset, page_size * 2);

	state.next_page(); //Will do nothing, full page was written

	auto hashes = std::move(state).get_page_hashes();
	EXPECT_EQ(hashes.size(), 4u * (hash_size + sizeof(std::uint32_t)));

	check_page1(hashes);
	check_page2(hashes, 0u);
	check_page3(hashes, page_size);
	check_page4(hashes, page_size * 2);
}
