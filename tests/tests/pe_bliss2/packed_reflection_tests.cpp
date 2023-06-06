#include <cstddef>
#include <cstdint>

#include "gtest/gtest.h"

#include "pe_bliss2/detail/packed_reflection.h"
#include "tests/pe_bliss2/test_structs.h"

using namespace pe_bliss::detail;

TEST(PackedReflectionTests, EmptyTest)
{
	struct empty {};
	EXPECT_EQ(packed_reflection::get_type_size<empty>(), 0u);
}

TEST(PackedReflectionTests, TypeSizeTest)
{
	EXPECT_EQ(packed_reflection::get_type_size<std::uint8_t>(),
		sizeof(std::uint8_t));
	EXPECT_EQ(packed_reflection::get_type_size<std::uint32_t[3u]>(),
		sizeof(std::uint32_t) * 3u);
	EXPECT_EQ(packed_reflection::get_type_size<simple>(), simple_size);
	EXPECT_EQ(packed_reflection::get_type_size<arrays>(), arrays_size);
	EXPECT_EQ(packed_reflection::get_type_size<nested>(), nested_size);
}

TEST(PackedReflectionTests, GetFieldOffsetTests)
{
	std::size_t offset = 0;
	EXPECT_EQ(packed_reflection::get_field_offset<&nested::a>(), offset);
	offset += sizeof(nested::a);
	EXPECT_EQ(packed_reflection::get_field_offset<&nested::b>(), offset);
	offset += simple_size;
	EXPECT_EQ(packed_reflection::get_field_offset<&nested::c>(), offset);
	offset += arrays_size;
	EXPECT_EQ(packed_reflection::get_field_offset<&nested::d>(), offset);
	offset += sizeof(std::uint8_t);
	EXPECT_EQ(packed_reflection::get_field_offset<&nested::e>(), offset);
	offset += simple_size * 10u;
	EXPECT_EQ(packed_reflection::get_field_offset<&nested::f>(), offset);
}
