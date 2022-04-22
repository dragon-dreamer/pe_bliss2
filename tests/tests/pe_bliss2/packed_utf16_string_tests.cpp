#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "buffers/input_buffer_state.h"
#include "buffers/input_memory_buffer.h"
#include "buffers/output_memory_ref_buffer.h"
#include "pe_bliss2/packed_utf16_string.h"

#include "tests/tests/pe_bliss2/pe_error_helper.h"

#include "utilities/generic_error.h"

namespace
{
static constexpr std::size_t size = 10;
static constexpr std::size_t max_physical_size
	= size * sizeof(pe_bliss::packed_utf16_string::string_type::value_type)
	+ sizeof(std::uint16_t);

auto create_test_string()
{
	pe_bliss::packed_utf16_string str;
	str.value().resize(size);
	str[0] = u'\xccdd';
	str[5] = u'\xaabb';
	str[7] = u'\x1122';
	str.sync_physical_size();
	return str;
}

constexpr std::array serialized_view{
	std::byte{123}, //dummy byte, data starts on the next byte
	std::byte{size},
	std::byte{},
	std::byte{0xdd},
	std::byte{0xcc},
	std::byte{},
	std::byte{},
	std::byte{},
	std::byte{},
	std::byte{},
	std::byte{},
	std::byte{},
	std::byte{},
	std::byte{0xbb},
	std::byte{0xaa},
	std::byte{},
	std::byte{},
	std::byte{0x22},
	std::byte{0x11},
	std::byte{},
	std::byte{},
	std::byte{},
	std::byte{}
};
} //namespace

TEST(PackedUtf16StringTests, ConstructionAndMetadataTest)
{
	pe_bliss::packed_utf16_string str;
	EXPECT_EQ(str.value(), pe_bliss::packed_utf16_string::string_type{});
	EXPECT_EQ(str.get_state(), buffers::serialized_data_state{});
	EXPECT_EQ(str.physical_size(), 0u);
	EXPECT_EQ(str.data_size(), 0u);
	EXPECT_FALSE(str.is_virtual());

	str.value().resize(size);
	EXPECT_EQ(str.physical_size(), 0u);
	EXPECT_EQ(str.data_size(), 0u);
	EXPECT_FALSE(str.is_virtual());

	str.set_physical_size(size - 1u);
	EXPECT_EQ(str.physical_size(), size - 1u);
	EXPECT_EQ(str.data_size(), size - 1u);

	str.set_physical_size(size + 100u);
	EXPECT_EQ(str.physical_size(), max_physical_size);
	EXPECT_EQ(str.data_size(), max_physical_size);

	str.set_physical_size(0u);
	str.sync_physical_size();
	EXPECT_EQ(str.physical_size(), max_physical_size);
	EXPECT_EQ(str.data_size(), max_physical_size);

	str.set_data_size(1u);
	EXPECT_EQ(str.physical_size(), max_physical_size);
	EXPECT_EQ(str.data_size(), max_physical_size);
	EXPECT_EQ(str.virtual_string_length(), (max_physical_size - sizeof(std::uint16_t))
		/ sizeof(pe_bliss::packed_utf16_string::string_type::value_type));

	str.set_data_size(size + 100u);
	EXPECT_EQ(str.physical_size(), max_physical_size);
	EXPECT_EQ(str.data_size(), size + 100u);
	EXPECT_EQ(str.virtual_string_length(), (size + 100u - sizeof(std::uint16_t))
		/ sizeof(pe_bliss::packed_utf16_string::string_type::value_type));

	buffers::serialized_data_state state;
	state.set_absolute_offset(1u);
	state.set_relative_offset(2u);
	state.set_buffer_pos(3u);
	str[1] = u'x';
	str.get_state() = state;

	auto copy1 = str;
	EXPECT_EQ(copy1.value(), str.value());
	EXPECT_EQ(copy1.get_state(), state);
	EXPECT_EQ(copy1.physical_size(), str.physical_size());
	EXPECT_EQ(copy1.data_size(), str.data_size());

	decltype(str) copy2;
	copy2 = copy1;
	EXPECT_EQ(copy2.value(), str.value());
	EXPECT_EQ(copy2.get_state(), state);
	EXPECT_EQ(copy2.physical_size(), str.physical_size());
	EXPECT_EQ(copy2.data_size(), str.data_size());
	EXPECT_EQ(copy2[1], u'x');
	EXPECT_EQ(std::as_const(copy2)[1], u'x');
}

TEST(PackedUtf16StringTests, SerializeTest1)
{
	auto str = create_test_string();

	std::vector<std::byte> serialized(max_physical_size + 1, std::byte{ 1 });
	{
		buffers::output_memory_ref_buffer buffer(serialized.data(), serialized.size());
		EXPECT_EQ(str.serialize(buffer, false), str.physical_size());
		EXPECT_TRUE(std::equal(serialized_view.cbegin() + sizeof(std::byte),
			serialized_view.cend(),
			serialized.cbegin()));
	}

	std::fill(serialized.begin(), serialized.end(), std::byte{ 1 });
	str.set_physical_size(str.physical_size() - 5u);
	{
		buffers::output_memory_ref_buffer buffer(serialized.data(),
			str.physical_size());
		EXPECT_EQ(str.serialize(buffer, false), str.physical_size());
		EXPECT_TRUE(std::equal(serialized_view.cbegin() + sizeof(std::byte),
			serialized_view.cbegin() + str.physical_size(),
			serialized.cbegin()));
		EXPECT_EQ(serialized[str.physical_size()], std::byte{ 1 });

		buffer.set_wpos(0);
		EXPECT_THROW(str.serialize(buffer, true), std::system_error);
	}

	str.set_data_size(str.physical_size() + 1000u);
	std::vector<std::byte> serialized2(str.data_size(), std::byte{ 1 });
	{
		buffers::output_memory_ref_buffer buffer(serialized2.data(),
			str.data_size());
		EXPECT_EQ(str.serialize(buffer, true), str.data_size());
		EXPECT_EQ(serialized2[0], std::byte{ str.virtual_string_length() & 0xff });
		EXPECT_EQ(serialized2[1], std::byte{ (str.virtual_string_length() >> CHAR_BIT) & 0xff });
		EXPECT_TRUE(std::equal(serialized_view.cbegin()
			+ sizeof(std::byte) + sizeof(std::uint16_t),
			serialized_view.cbegin() + str.physical_size(),
			serialized2.cbegin() + sizeof(std::uint16_t)));
		EXPECT_TRUE(std::all_of(serialized2.cbegin() + str.physical_size(),
			serialized2.cend(), [](auto elem) { return elem == std::byte{}; }));
	}
}

TEST(PackedUtf16StringTests, SerializeTest2)
{
	auto str = create_test_string();

	std::vector<std::byte> serialized(max_physical_size + 1001u, std::byte{ 1 });

	expect_throw_pe_error([&] {
		str.serialize(serialized.data(), str.physical_size() - 1u, false); },
		utilities::generic_errc::buffer_overrun);

	EXPECT_EQ(serialized[0], std::byte{ 1 });

	{
		EXPECT_EQ(str.serialize(serialized.data(), str.physical_size(), false),
			str.physical_size());
		EXPECT_TRUE(std::equal(serialized_view.cbegin() + sizeof(std::byte),
			serialized_view.cbegin() + str.physical_size(),
			serialized.cbegin()));
		EXPECT_EQ(serialized[str.physical_size()], std::byte{ 1 });
	}

	std::fill(serialized.begin(), serialized.end(), std::byte{ 1 });
	str.set_data_size(str.physical_size() + 1000u);

	expect_throw_pe_error([&] {
		str.serialize(serialized.data(), str.physical_size(), true); },
		utilities::generic_errc::buffer_overrun);
	EXPECT_EQ(serialized[0], std::byte{ 1 });
	EXPECT_EQ(str.serialize(serialized.data(), str.data_size(), true),
		str.data_size());
	EXPECT_EQ(serialized[0], std::byte{ str.virtual_string_length() & 0xff });
	EXPECT_EQ(serialized[1], std::byte{ (str.virtual_string_length() >> CHAR_BIT) & 0xff });
	EXPECT_TRUE(std::equal(serialized_view.cbegin()
		+ sizeof(std::byte) + sizeof(std::uint16_t),
		serialized_view.cbegin() + str.physical_size(),
		serialized.cbegin() + sizeof(std::uint16_t)));
	EXPECT_EQ(serialized[str.data_size() - 1], std::byte{});
	EXPECT_EQ(serialized[str.data_size()], std::byte{ 1 });
}

TEST(PackedUtf16StringTests, DeserializeTest)
{
	pe_bliss::packed_utf16_string str;

	static constexpr std::size_t buffer_pos = 1u;
	static constexpr std::size_t absolute_offset = 2u;
	static constexpr std::size_t relative_offset = 3u;
	const auto deserialized_string = std::move(create_test_string()).value();

	{
		buffers::input_memory_buffer buffer(
			serialized_view.data(), serialized_view.size());
		buffer.set_absolute_offset(absolute_offset);
		buffer.set_relative_offset(relative_offset);

		expect_throw_pe_error([&] {
			str.deserialize(buffer, false); },
			utilities::generic_errc::buffer_overrun);
		EXPECT_EQ(str.value(), pe_bliss::packed_utf16_string::string_type{});
		EXPECT_EQ(str.get_state(), buffers::serialized_data_state{});
		EXPECT_EQ(str.physical_size(), 0u);
		EXPECT_EQ(str.data_size(), 0u);
		EXPECT_FALSE(str.is_virtual());

		buffer.set_rpos(buffer_pos);
		EXPECT_NO_THROW(str.deserialize(buffer, false));

		EXPECT_EQ(str.get_state().absolute_offset(), absolute_offset + buffer_pos);
		EXPECT_EQ(str.get_state().relative_offset(), relative_offset + buffer_pos);
		EXPECT_EQ(str.get_state().buffer_pos(), buffer_pos);
		EXPECT_FALSE(str.is_virtual());
		EXPECT_EQ(str.physical_size(), max_physical_size);
		EXPECT_EQ(str.data_size(), max_physical_size);
		EXPECT_EQ(str.value(), deserialized_string);
	}

	{
		static constexpr std::size_t cut_size = 5u;
		buffers::input_memory_buffer buffer(
			serialized_view.data() + sizeof(std::byte{}),
			serialized_view.size() - sizeof(std::byte{}) - cut_size);
		EXPECT_NO_THROW(str.deserialize(buffer, true));
		EXPECT_EQ(buffer.rpos(), buffer.size());
		EXPECT_TRUE(str.is_virtual());
		EXPECT_EQ(str.physical_size(), buffer.size());
		EXPECT_EQ(str.data_size(), max_physical_size);

		auto cut_deserialized_string = deserialized_string;
		cut_deserialized_string.resize(cut_deserialized_string.size() - 3);
		cut_deserialized_string.push_back(u'\x0022');
		EXPECT_EQ(str.value(), cut_deserialized_string);
	}

	{
		buffers::input_memory_buffer buffer(
			serialized_view.data() + sizeof(std::byte{}), 1u);
		EXPECT_NO_THROW(str.deserialize(buffer, true));
		EXPECT_EQ(buffer.rpos(), buffer.size());
		EXPECT_TRUE(str.is_virtual());
		EXPECT_EQ(str.physical_size(), buffer.size());
		EXPECT_EQ(str.data_size(), max_physical_size);
		EXPECT_EQ(str.value(), pe_bliss::packed_utf16_string::string_type{});
	}
}
