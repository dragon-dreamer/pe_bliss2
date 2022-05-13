#include <cstdint>

#include <boost/endian.hpp>

#include "gtest/gtest.h"

#include "pe_bliss2/detail/endian_convert.h"

using namespace pe_bliss::detail;

TEST(EndianConvertTests, SameEndianTests)
{
	static constexpr std::uint32_t initial_value = 0x12345678;
	std::uint32_t value = initial_value;

	convert_endianness<boost::endian::order::big,
		boost::endian::order::big>(value);
	EXPECT_EQ(value, initial_value);

	convert_endianness<boost::endian::order::little,
		boost::endian::order::little>(value);
	EXPECT_EQ(value, initial_value);

	convert_endianness<boost::endian::order::native,
		boost::endian::order::native>(value);
	EXPECT_EQ(value, initial_value);
}

TEST(PackedSerializationTests, LittleToBigTests)
{
	static constexpr std::uint32_t initial_value = 0x12345678u;
	static constexpr std::uint32_t reversed_value = 0x78563412u;
	std::uint32_t value = initial_value;

	convert_endianness<boost::endian::order::little,
		boost::endian::order::big>(value);
	EXPECT_EQ(value, reversed_value);

	convert_endianness<boost::endian::order::big,
		boost::endian::order::little>(value);
	EXPECT_EQ(value, initial_value);
}
