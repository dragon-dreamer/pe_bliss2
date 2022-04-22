#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "buffers/input_buffer_state.h"
#include "buffers/input_memory_buffer.h"
#include "buffers/output_memory_buffer.h"
#include "pe_bliss2/packed_c_string.h"

#include "tests/tests/pe_bliss2/pe_error_helper.h"

#include "utilities/generic_error.h"

namespace
{
constexpr const char* test_string = "test string";
constexpr auto test_string_length = std::char_traits<char>::length(test_string);
} //namespace

TEST(PackedCStringTests, ConstructionTest)
{
	pe_bliss::packed_c_string empty;
	EXPECT_TRUE(empty.value().empty());

	pe_bliss::packed_c_string str(test_string);
	EXPECT_EQ(str.value(), test_string);

	auto copy(str);
	EXPECT_EQ(str.value(), test_string);

	empty = str;
	EXPECT_EQ(empty.value(), test_string);
	EXPECT_EQ(str.value(), test_string);

	empty = std::move(str);
	EXPECT_EQ(empty.value(), test_string);
}

TEST(PackedCStringTests, StateTest)
{
	pe_bliss::packed_c_string empty;
	EXPECT_EQ(empty.get_state(), buffers::serialized_data_state{});
	static constexpr std::size_t absolute_offset = 1u;
	static constexpr std::size_t relative_offset = 2u;
	static constexpr std::size_t buffer_pos = 3u;
	empty.get_state().set_absolute_offset(absolute_offset);
	empty.get_state().set_relative_offset(relative_offset);
	empty.get_state().set_buffer_pos(buffer_pos);

	auto copy1(empty);
	EXPECT_EQ(copy1.get_state().absolute_offset(), absolute_offset);
	EXPECT_EQ(copy1.get_state().relative_offset(), relative_offset);
	EXPECT_EQ(copy1.get_state().buffer_pos(), buffer_pos);

	pe_bliss::packed_c_string copy2;
	copy2 = copy1;
	EXPECT_EQ(copy2.get_state().absolute_offset(), absolute_offset);
	EXPECT_EQ(copy2.get_state().relative_offset(), relative_offset);
	EXPECT_EQ(copy2.get_state().buffer_pos(), buffer_pos);
}

TEST(PackedCStringTests, VirtualNullbyteTest)
{
	pe_bliss::packed_c_string empty;
	EXPECT_FALSE(empty.is_virtual());
	EXPECT_EQ(empty.physical_size(), 1u);
	EXPECT_NO_THROW(empty.set_virtual_nullbyte(true));
	EXPECT_TRUE(empty.is_virtual());
	EXPECT_EQ(empty.physical_size(), 0u);

	auto copy1(empty);
	EXPECT_TRUE(copy1.is_virtual());
	EXPECT_EQ(copy1.physical_size(), 0u);

	pe_bliss::packed_c_string copy2;
	copy2 = copy1;
	EXPECT_TRUE(copy2.is_virtual());
	EXPECT_EQ(copy2.physical_size(), 0u);

	copy2 = test_string;
	EXPECT_FALSE(copy2.is_virtual());
	EXPECT_EQ(copy2.physical_size(), test_string_length + 1u);
}

TEST(PackedCStringTests, SerializeTest1)
{
	pe_bliss::packed_c_string str(test_string);

	std::vector<std::byte> serialized;
	buffers::output_memory_buffer buffer(serialized);
	EXPECT_EQ(str.serialize(buffer, false), test_string_length + 1u);
	EXPECT_EQ(serialized.size(), test_string_length + 1u);
	EXPECT_EQ(std::memcmp(serialized.data(), test_string, serialized.size()), 0);

	serialized.clear();
	buffer.set_wpos(0u);
	str.set_virtual_nullbyte(true);
	EXPECT_EQ(str.serialize(buffer, false), test_string_length);
	EXPECT_EQ(serialized.size(), test_string_length);
	EXPECT_EQ(std::memcmp(serialized.data(), test_string, serialized.size()), 0);

	serialized.clear();
	buffer.set_wpos(0u);
	EXPECT_EQ(str.serialize(buffer, true), test_string_length + 1u);
	EXPECT_EQ(serialized.size(), test_string_length + 1u);
	EXPECT_EQ(std::memcmp(serialized.data(), test_string, serialized.size()), 0);
}

TEST(PackedCStringTests, SerializeTest2)
{
	pe_bliss::packed_c_string str(test_string);

	std::array<std::byte, test_string_length + 1u> serialized{};
	expect_throw_pe_error([&] {
		str.serialize(serialized.data(), test_string_length, false); },
		utilities::generic_errc::buffer_overrun);
	EXPECT_EQ(str.serialize(serialized.data(), test_string_length + 1u, false),
		test_string_length + 1u);
	EXPECT_EQ(std::memcmp(serialized.data(), test_string,
		test_string_length + 1u), 0);

	serialized = {};
	str.set_virtual_nullbyte(true);
	EXPECT_EQ(str.serialize(serialized.data(), test_string_length, false),
		test_string_length);
	EXPECT_EQ(std::memcmp(serialized.data(), test_string, serialized.size()), 0);

	serialized = {};
	EXPECT_EQ(str.serialize(serialized.data(), test_string_length + 100u, true),
		test_string_length + 1u);
	EXPECT_EQ(std::memcmp(serialized.data(), test_string, serialized.size()), 0);
}

TEST(PackedCStringTests, DeserializeTest)
{
	pe_bliss::packed_c_string str(test_string);

	static constexpr std::size_t absolute_offset = 1u;
	static constexpr std::size_t relative_offset = 2u;
	static constexpr std::size_t buffer_pos = 3u;

	{
		str.set_virtual_nullbyte(true);

		buffers::input_memory_buffer buffer(reinterpret_cast<const std::byte*>(test_string),
			test_string_length + 1u);
		buffer.set_rpos(buffer_pos);
		buffer.set_absolute_offset(absolute_offset);
		buffer.set_relative_offset(relative_offset);
		EXPECT_NO_THROW(str.deserialize(buffer, false));
		EXPECT_EQ(str.get_state().absolute_offset(), absolute_offset + buffer_pos);
		EXPECT_EQ(str.get_state().relative_offset(), relative_offset + buffer_pos);
		EXPECT_EQ(str.get_state().buffer_pos(), buffer_pos);
		EXPECT_FALSE(str.is_virtual());
		EXPECT_EQ(str.value(), test_string + buffer_pos);
	}

	{
		buffers::input_memory_buffer buffer(reinterpret_cast<const std::byte*>(test_string),
			test_string_length);
		expect_throw_pe_error([&] {
			str.deserialize(buffer, false); },
			utilities::generic_errc::buffer_overrun);
		EXPECT_EQ(str.get_state().absolute_offset(), absolute_offset + buffer_pos);
		EXPECT_EQ(str.get_state().relative_offset(), relative_offset + buffer_pos);
		EXPECT_EQ(str.get_state().buffer_pos(), buffer_pos);
		EXPECT_FALSE(str.is_virtual());
		EXPECT_EQ(str.value(), test_string + buffer_pos);
	}

	{
		buffers::input_memory_buffer buffer(reinterpret_cast<const std::byte*>(test_string),
			test_string_length);
		EXPECT_NO_THROW(str.deserialize(buffer, true));
		EXPECT_EQ(str.get_state().absolute_offset(), 0u);
		EXPECT_EQ(str.get_state().relative_offset(), 0u);
		EXPECT_EQ(str.get_state().buffer_pos(), 0u);
		EXPECT_TRUE(str.is_virtual());
		EXPECT_EQ(str.value(), test_string);
	}
}
