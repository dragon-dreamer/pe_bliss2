#include "gtest/gtest.h"

#include <climits>
#include <cstddef>
#include <system_error>
#include <vector>

#include "pe_bliss2/bit_stream.h"

namespace
{
template<typename T>
class BitStreamTests : public testing::Test
{
public:
	using type = T;
};

using tested_types = ::testing::Types<std::byte, std::uint8_t,
	std::uint16_t, std::uint32_t, std::uint64_t>;
} //namespace

TYPED_TEST_SUITE(BitStreamTests, tested_types);

TYPED_TEST(BitStreamTests, ConstructionTest)
{
	using type = typename TestFixture::type;
	std::vector<type> bytes{ type{1}, type{2}, type{3} };
	const std::size_t bit_count = bytes.size() * CHAR_BIT * sizeof(type);
	pe_bliss::bit_stream stream(bytes);
	EXPECT_EQ(stream.get_bit_count(), bit_count);
	EXPECT_EQ(stream.get_pos(), 0u);
	ASSERT_NO_THROW(stream.set_pos(bit_count));
	EXPECT_EQ(stream.get_pos(), bit_count);
	EXPECT_THROW(stream.set_pos(bit_count + 1), std::system_error);
	EXPECT_EQ(stream.get_pos(), bit_count);
}

TEST(BitStreamTests, Uint8Test)
{
	const std::vector<std::uint8_t> bytes{ 0b11001100, 0b10011100, 0b00011010 };

	pe_bliss::bit_stream stream(bytes);
	EXPECT_EQ(stream.read(24), 0b00011010'10011100'11001100);

	ASSERT_NO_THROW(stream.set_pos(0u));
	for (int expected : { 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1,
		0, 1, 0, 1, 1, 0, 0, 0 }) {
		EXPECT_EQ(stream.read(1), expected);
	}

	ASSERT_NO_THROW(stream.set_pos(0u));
	for (int expected : { 0b100, 0b001, 0b011, 0b110, 0b001,
		0b101, 0b110, 0b000 }) {
		EXPECT_EQ(stream.read(3), expected);
	}

	ASSERT_NO_THROW(stream.set_pos(0u));
	for (int expected : { 0b1001100, 0b0111001, 0b1101010 }) {
		EXPECT_EQ(stream.read(7), expected);
	}

	EXPECT_THROW((void)stream.read(7), std::system_error);
	EXPECT_EQ(stream.get_pos(), 21u);
}

TEST(BitStreamTests, ByteTest)
{
	const std::vector<std::byte> bytes{ std::byte{0b11001100},
		std::byte{0b10011100}, std::byte{0b00011010},
		std::byte{0b10100111}, std::byte{0b10010011},
		std::byte{0b11101111}, std::byte{0b10000010},
		std::byte{0b11111100}, std::byte{0b11111111} };

	pe_bliss::bit_stream stream(bytes);
	EXPECT_THROW((void)stream.read<std::uint8_t>(9), std::system_error);
	EXPECT_EQ(stream.get_pos(), 0u);

	EXPECT_EQ(stream.read<std::uint8_t>(8), 0b11001100);
	EXPECT_EQ(stream.read<std::uint16_t>(16), 0b0001101010011100);
	ASSERT_NO_THROW(stream.set_pos(0u));
	EXPECT_EQ(stream.read<std::uint64_t>(64),
		0b11111100'10000010'11101111'10010011'10100111'00011010'10011100'11001100ull);

	ASSERT_NO_THROW(stream.set_pos(2u));
	EXPECT_EQ(stream.read<std::uint16_t>(16), 0b1010011100110011);
	ASSERT_NO_THROW(stream.set_pos(2u));
	EXPECT_EQ(stream.read<std::uint64_t>(64),
		0b11'11111100'10000010'11101111'10010011'10100111'00011010'10011100'110011ull);
}

TEST(BitStreamTests, Uint16Test)
{
	const std::vector<std::uint16_t> bytes{ 0b1100110011110110,
		0b1001110000111001, 0b0001101011111101 };

	pe_bliss::bit_stream stream(bytes);
	EXPECT_EQ(stream.read(32), 0b1001110000111001'1100110011110110);

	ASSERT_NO_THROW(stream.set_pos(0u));
	for (int expected : { 0, 1, 1, 0, 1, 1, 1, 1, 0, 0,
		1, 1, 0, 0, 1, 1, 1, 0, 0 }) {
		EXPECT_EQ(stream.read(1), expected);
	}

	ASSERT_NO_THROW(stream.set_pos(0u));
	for (int expected : { 0b110, 0b110, 0b011, 0b110, 0b100, 0b011,
		0b110 }) {
		EXPECT_EQ(stream.read(3), expected);
	}

	ASSERT_NO_THROW(stream.set_pos(0u));
	for (int expected : { 0b1110110, 0b0011001, 0b1100111,
		0b1100001 }) {
		EXPECT_EQ(stream.read(7), expected);
	}
}

TEST(BitStreamTests, Uint64Test)
{
	const std::vector<std::uint64_t> bytes{ 
		0b11111100'10000010'11101111'10010011'10100111'00011010'10011100'11001111ull,
		0b10000101'10010111'10100011'01111001'10100000'11100101'10010110'01011110ull };

	pe_bliss::bit_stream stream(bytes);
	EXPECT_EQ(stream.read(32), 0b10100111'00011010'10011100'11001111);

	ASSERT_NO_THROW(stream.set_pos(0u));
	for (int expected : { 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0 }) {
		EXPECT_EQ(stream.read(1), expected);
	}

	ASSERT_NO_THROW(stream.set_pos(54u));
	for (int expected : { 0b010, 0b110, 0b111, 0b101 }) {
		EXPECT_EQ(stream.read(3), expected);
	}

	ASSERT_NO_THROW(stream.set_pos(0u));
	for (int expected : { 0b1001111, 0b0111001, 0b1101010 }) {
		EXPECT_EQ(stream.read(7), expected);
	}

	ASSERT_NO_THROW(stream.set_pos(64u));
	EXPECT_EQ(stream.read<std::uint64_t>(64),
		0b10000101'10010111'10100011'01111001'10100000'11100101'10010110'01011110ull);

	ASSERT_NO_THROW(stream.set_pos(61u));
	EXPECT_EQ(stream.read<std::uint64_t>(64),
		0b00101'10010111'10100011'01111001'10100000'11100101'10010110'01011110'111ull);
}
