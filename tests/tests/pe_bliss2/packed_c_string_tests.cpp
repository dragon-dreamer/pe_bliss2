#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "gtest/gtest.h"

#include "buffers/input_buffer_state.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_memory_buffer.h"
#include "buffers/input_virtual_buffer.h"
#include "buffers/output_memory_buffer.h"
#include "pe_bliss2/packed_c_string.h"

#include "tests/pe_bliss2/pe_error_helper.h"

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
	ASSERT_EQ(str.serialize(buffer, false), test_string_length + 1u);
	ASSERT_EQ(serialized.size(), test_string_length + 1u);
	EXPECT_EQ(std::memcmp(serialized.data(), test_string, serialized.size()), 0);

	serialized.clear();
	buffer.set_wpos(0u);
	str.set_virtual_nullbyte(true);
	ASSERT_EQ(str.serialize(buffer, false), test_string_length);
	ASSERT_EQ(serialized.size(), test_string_length);
	EXPECT_EQ(std::memcmp(serialized.data(), test_string, serialized.size()), 0);

	serialized.clear();
	buffer.set_wpos(0u);
	ASSERT_EQ(str.serialize(buffer, true), test_string_length + 1u);
	ASSERT_EQ(serialized.size(), test_string_length + 1u);
	EXPECT_EQ(std::memcmp(serialized.data(), test_string, serialized.size()), 0);
}

TEST(PackedCStringTests, SerializeTest2)
{
	pe_bliss::packed_c_string str(test_string);

	std::array<std::byte, test_string_length + 1u> serialized{};
	expect_throw_pe_error([&] {
		str.serialize(serialized.data(), test_string_length, false); },
		utilities::generic_errc::buffer_overrun);
	ASSERT_EQ(str.serialize(serialized.data(), test_string_length + 1u, false),
		test_string_length + 1u);
	EXPECT_EQ(std::memcmp(serialized.data(), test_string,
		test_string_length + 1u), 0);

	serialized = {};
	str.set_virtual_nullbyte(true);
	ASSERT_EQ(str.serialize(serialized.data(), test_string_length, false),
		test_string_length);
	EXPECT_EQ(std::memcmp(serialized.data(), test_string, serialized.size()), 0);

	serialized = {};
	ASSERT_EQ(str.serialize(serialized.data(), test_string_length + 100u, true),
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

		buffers::input_memory_buffer buffer(
			reinterpret_cast<const std::byte*>(test_string),
			test_string_length + 1u);
		buffers::input_buffer_stateful_wrapper_ref ref(buffer);
		ref.set_rpos(buffer_pos);
		buffer.set_absolute_offset(absolute_offset);
		buffer.set_relative_offset(relative_offset);
		ASSERT_NO_THROW(str.deserialize(ref, false));
		EXPECT_EQ(str.get_state().absolute_offset(), absolute_offset + buffer_pos);
		EXPECT_EQ(str.get_state().relative_offset(), relative_offset + buffer_pos);
		EXPECT_EQ(str.get_state().buffer_pos(), buffer_pos);
		EXPECT_FALSE(str.is_virtual());
		EXPECT_EQ(str.value(), test_string + buffer_pos);
	}

	{
		buffers::input_memory_buffer buffer(
			reinterpret_cast<const std::byte*>(test_string),
			test_string_length);
		buffers::input_buffer_stateful_wrapper_ref ref(buffer);
		// Not allowed to go out of the buffer bounds
		EXPECT_THROW(str.deserialize(ref, true), std::system_error);
	}

	auto buffer_ptr = std::make_shared<buffers::input_memory_buffer>(
		reinterpret_cast<const std::byte*>(test_string),
		test_string_length);
	buffers::input_virtual_buffer virtual_buffer(buffer_ptr, 1u); //virtual nullbyte
	buffers::input_buffer_stateful_wrapper_ref ref(virtual_buffer);

	{
		expect_throw_pe_error([&] {
			str.deserialize(ref, false); },
			utilities::generic_errc::buffer_overrun);
		EXPECT_EQ(str.get_state().absolute_offset(), absolute_offset + buffer_pos);
		EXPECT_EQ(str.get_state().relative_offset(), relative_offset + buffer_pos);
		EXPECT_EQ(str.get_state().buffer_pos(), buffer_pos);
		EXPECT_FALSE(str.is_virtual());
		EXPECT_EQ(str.value(), test_string + buffer_pos);
	}

	{
		ref.set_rpos(0);
		ASSERT_NO_THROW(str.deserialize(ref, true));
		EXPECT_EQ(str.get_state().absolute_offset(), 0u);
		EXPECT_EQ(str.get_state().relative_offset(), 0u);
		EXPECT_EQ(str.get_state().buffer_pos(), 0u);
		EXPECT_TRUE(str.is_virtual());
		EXPECT_EQ(str.value(), test_string);
	}
}

TEST(PackedCStringTests, DeserializeLimitTest)
{
	buffers::input_memory_buffer buffer(
		reinterpret_cast<const std::byte*>(test_string),
		test_string_length + 1u);
	
	buffers::input_buffer_stateful_wrapper_ref ref(buffer);
	pe_bliss::packed_c_string str;
	EXPECT_NO_THROW(str.deserialize(ref, true, test_string_length + 1u));
	EXPECT_EQ(str.value(), test_string);

	EXPECT_THROW(str.deserialize(ref, true, test_string_length), std::system_error);
	EXPECT_EQ(str.value(), test_string);
}
