#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <tuple>
#include <type_traits>

#include <boost/endian.hpp>

#include "gtest/gtest.h"

#include "pe_bliss2/detail/packed_serialization.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "tests/pe_bliss2/test_structs.h"

using namespace pe_bliss::detail;

namespace
{
struct empty
{
	friend auto operator<=>(const empty&, const empty&) = default;
};

constexpr auto opposite_endianness
	= boost::endian::order::native == boost::endian::order::little
	? boost::endian::order::big
	: boost::endian::order::little;
using serializer_to_opposite = packed_serialization<opposite_endianness>;
using serializer_to_same = packed_serialization<>;

template<typename Serializer>
struct serialize_serializer
{
	template<typename T>
	static auto serialize(const T& value, std::byte* data)
	{
		return Serializer::serialize(value, data);
	}
};

template<typename Serializer>
struct deserialize_deserializer
{
	template<typename T>
	static auto deserialize(T& value, const std::byte* data)
	{
		return Serializer::deserialize(value, data);
	}
};

template<auto FieldPtr, typename Serializer>
struct serialize_until_field_serializer
{
	template<typename T>
	static auto serialize(const T& value, std::byte* data)
	{
		return Serializer::template serialize_until<FieldPtr>(value, data);
	}
};

template<std::size_t Size, typename Serializer>
struct serialize_until_size_serializer
{
	template<typename T>
	static auto serialize(const T& value, std::byte* data)
	{
		return Serializer::serialize_until(value, data, Size);
	}
};

template<auto FieldPtr, typename Serializer>
struct deserialize_until_field_deserializer
{
	template<typename T>
	static auto deserialize(T& value, const std::byte* data)
	{
		return Serializer::template deserialize_until<FieldPtr>(value, data);
	}
};

template<std::size_t Size, typename Serializer>
struct deserialize_until_size_deserializer
{
	template<typename T>
	static auto deserialize(T& value, const std::byte* data)
	{
		return Serializer::deserialize_until(value, data, Size);
	}
};

template<template <typename> typename Serializer = serialize_serializer,
	typename Value>
void test_serialization(const Value& value, std::string_view expected,
	std::string_view expected_reversed)
{
	std::array<std::byte, packed_reflection::get_type_size<Value>()> buf{};
	ASSERT_GE(buf.size(), expected.size());
	ASSERT_EQ(expected.size(), expected_reversed.size());

	ASSERT_EQ(Serializer<serializer_to_same>::serialize(
		value, buf.data()), buf.data() + expected.size());
	EXPECT_TRUE(std::equal(buf.cbegin(), buf.cbegin() + expected.size(),
		boost::endian::order::native == boost::endian::order::little
		? reinterpret_cast<const std::byte*>(expected_reversed.data())
		: reinterpret_cast<const std::byte*>(expected.data())));

	buf = {};
	ASSERT_EQ(Serializer<serializer_to_opposite>::serialize(
		value, buf.data()), buf.data() + expected.size());
	EXPECT_TRUE(std::equal(buf.cbegin(), buf.cbegin() + expected.size(),
		boost::endian::order::native == boost::endian::order::little
		? reinterpret_cast<const std::byte*>(expected.data())
		: reinterpret_cast<const std::byte*>(expected_reversed.data())));
}

template<template <typename> typename Deserializer = deserialize_deserializer,
	typename Value>
void test_deserialization(std::string_view serialized,
	std::string_view serialized_reversed, const Value& target_value)
{
	auto size = serialized.size();
	ASSERT_EQ(size, serialized_reversed.size());

	Value value {};
	auto deserialize_buf
		= boost::endian::order::native == boost::endian::order::little
		? reinterpret_cast<const std::byte*>(serialized_reversed.data())
		: reinterpret_cast<const std::byte*>(serialized.data());
	ASSERT_EQ(Deserializer<serializer_to_same>::deserialize(
		value, deserialize_buf), deserialize_buf + size);
	EXPECT_EQ(value, target_value);

	value = {};
	deserialize_buf
		= boost::endian::order::native == boost::endian::order::little
		? reinterpret_cast<const std::byte*>(serialized.data())
		: reinterpret_cast<const std::byte*>(serialized_reversed.data());
	ASSERT_EQ(Deserializer<serializer_to_opposite>::deserialize(
		value, deserialize_buf), deserialize_buf + size);
	EXPECT_EQ(value, target_value);
}

template<typename Value>
void test_serialization(const Value& value, std::string_view expected,
	std::string_view expected_reversed, std::size_t& index)
{
	test_serialization(value, expected, expected_reversed);
	++index;
}

template<typename Value>
void test_deserialization(std::string_view serialized,
	std::string_view serialized_reversed, const Value& target_value,
	std::size_t& index)
{
	test_deserialization(serialized, serialized_reversed, target_value);
	++index;
}

using test_types = std::tuple<empty, std::uint8_t, std::uint16_t,
	std::uint32_t, std::uint64_t, simple, nested_short>;

constexpr std::uint32_t simple_array[3]{
	0x12345678u, 0xaabbccddu, 0xabcdef00u };

constexpr test_types serialized_values{
	{},
	0x12u,
	0x1234u,
	0x12345678u,
	0x1234567890abcdefull,
	{
		0xabu,
		0x12345678u,
		0xaabbu,
		0x1122334455667788u
	},
	{
		0x12345678u,
		{ 0x1122u, 0x3344u, 0x5566u },
		{ 0xabu, 0x12345678u, 0xaabbu, 0x1122334455667788ull,
			0xcdu, 0x23456789u, 0xccddu, 0x2233445566778899ull,
			0xdeu, 0x34567890u, 0xeeffu, 0x3344556677889900ull,
		  0xefu, 0x45678901u, 0x1122u, 0x445566778899aabbull,
			0xf1u, 0x56789012u, 0x3344u, 0x5566778899aabbccull,
			0x12u, 0x67890123u, 0x5566u, 0x778899aabbccddeeull
		},
		{ 0xffu, 0xeeu }
	}
};

using namespace std::string_view_literals;

constexpr std::array serialized_representations{
	""sv,
	"\x12"sv,
	"\x12\x34"sv,
	"\x12\x34\x56\x78"sv,
	"\x12\x34\x56\x78\x90\xab\xcd\xef"sv,
	"\xab" "\x12\x34\x56\x78" "\xaa\xbb" "\x11\x22\x33\x44\x55\x66\x77\x88"sv,
	"\x12\x34\x56\x78"
		"\x11\x22" "\x33\x44" "\x55\x66"
		"\xab" "\x12\x34\x56\x78" "\xaa\xbb" "\x11\x22\x33\x44\x55\x66\x77\x88"
		"\xcd" "\x23\x45\x67\x89" "\xcc\xdd" "\x22\x33\x44\x55\x66\x77\x88\x99"
		"\xde" "\x34\x56\x78\x90" "\xee\xff" "\x33\x44\x55\x66\x77\x88\x99\x00"
		"\xef" "\x45\x67\x89\x01" "\x11\x22" "\x44\x55\x66\x77\x88\x99\xaa\xbb"
		"\xf1" "\x56\x78\x90\x12" "\x33\x44" "\x55\x66\x77\x88\x99\xaa\xbb\xcc"
		"\x12" "\x67\x89\x01\x23" "\x55\x66" "\x77\x88\x99\xaa\xbb\xcc\xdd\xee"
		"\xff" "\xee"sv
};

constexpr std::array serialized_representations_reversed{
	""sv,
	"\x12"sv,
	"\x34\x12"sv,
	"\x78\x56\x34\x12"sv,
	"\xef\xcd\xab\x90\x78\x56\x34\x12"sv,
	"\xab" "\x78\x56\x34\x12" "\xbb\xaa" "\x88\x77\x66\x55\x44\x33\x22\x11"sv,
	"\x78\x56\x34\x12"
		"\x22\x11" "\x44\x33" "\x66\x55"
		"\xab" "\x78\x56\x34\x12" "\xbb\xaa" "\x88\x77\x66\x55\x44\x33\x22\x11"
		"\xcd" "\x89\x67\x45\x23" "\xdd\xcc" "\x99\x88\x77\x66\x55\x44\x33\x22"
		"\xde" "\x90\x78\x56\x34" "\xff\xee" "\x00\x99\x88\x77\x66\x55\x44\x33"
		"\xef" "\x01\x89\x67\x45" "\x22\x11" "\xbb\xaa\x99\x88\x77\x66\x55\x44"
		"\xf1" "\x12\x90\x78\x56" "\x44\x33" "\xcc\xbb\xaa\x99\x88\x77\x66\x55"
		"\x12" "\x23\x01\x89\x67" "\x66\x55" "\xee\xdd\xcc\xbb\xaa\x99\x88\x77"
		"\xff" "\xee"sv
};

constexpr auto simple_array_serialized_representation
	= "\x12\x34\x56\x78" "\xaa\xbb\xcc\xdd" "\xab\xcd\xef\x00"sv;
constexpr auto simple_array_serialized_representation_reversed
	= "\x78\x56\x34\x12" "\xdd\xcc\xbb\xaa" "\x00\xef\xcd\xab"sv;

static_assert(serialized_representations.size()
	== std::tuple_size_v<test_types>);
static_assert(serialized_representations_reversed.size()
	== std::tuple_size_v<test_types>);
} //namespace

TEST(PackedSerializationTests, SerializeTests)
{
	std::size_t index = 0;
	std::apply([&index](const auto&... value) {
		(..., test_serialization(value,
			serialized_representations[index],
			serialized_representations_reversed[index],
			index));
	}, serialized_values);
}

TEST(PackedSerializationTests, SerializeUint32ArrayTest)
{
	test_serialization(simple_array,
		simple_array_serialized_representation,
		simple_array_serialized_representation_reversed);
}

TEST(PackedSerializationTests, DeserializeTests)
{
	std::size_t index = 0;
	std::apply([&index](const auto&... value) {
		(..., test_deserialization(
			serialized_representations[index],
			serialized_representations_reversed[index],
			value, index));
	}, serialized_values);
}

namespace
{
constexpr auto size_simple_until_c
	= packed_reflection::get_type_size<decltype(simple::a)>()
	+ packed_reflection::get_type_size<decltype(simple::b)>()
	+ packed_reflection::get_type_size<decltype(simple::c)>();
constexpr auto size_nested_until_b
	= packed_reflection::get_type_size<decltype(nested_short::a)>()
	+ packed_reflection::get_type_size<decltype(nested_short::b)>();

template<typename T>
using serializer_simple_until_c = serialize_until_field_serializer<&simple::c, T>;
template<typename T>
using serializer_nested_until_b = serialize_until_field_serializer<&nested_short::b, T>;
template<typename T>
using serializer_simple_until_c_size = serialize_until_size_serializer<size_simple_until_c, T>;
template<typename T>
using serializer_nested_until_b_size = serialize_until_size_serializer<size_nested_until_b, T>;

template<typename T>
using deserializer_simple_until_c = deserialize_until_field_deserializer<&simple::c, T>;
template<typename T>
using deserializer_nested_until_b = deserialize_until_field_deserializer<&nested_short::b, T>;
template<typename T>
using deserializer_simple_until_c_size = deserialize_until_size_deserializer<size_simple_until_c, T>;
template<typename T>
using deserializer_nested_until_b_size = deserialize_until_size_deserializer<size_nested_until_b, T>;

template<template<typename> typename... T, typename Func>
void run_for_types(Func&& func)
{
	(..., func.template operator()<T>());
}
} //namespace

TEST(PackedSerializationTests, SerializeUntilFieldTest1)
{
	run_for_types<serializer_simple_until_c, serializer_simple_until_c_size>(
		[]<template<typename> typename Serializer> {
			test_serialization<Serializer>(
				std::get<simple>(serialized_values),
				serialized_representations[5].substr(0, size_simple_until_c),
				serialized_representations_reversed[5].substr(0, size_simple_until_c));
		}
	);
}

TEST(PackedSerializationTests, SerializeUntilFieldTest2)
{
	run_for_types<serializer_nested_until_b, serializer_nested_until_b_size>(
		[]<template<typename> typename Serializer> {
			test_serialization<Serializer>(
				std::get<nested_short>(serialized_values),
				serialized_representations[6].substr(0, size_nested_until_b),
				serialized_representations_reversed[6].substr(0, size_nested_until_b));
		}
	);
}

TEST(PackedSerializationTests, DeserializeUntilFieldTest1)
{
	run_for_types<deserializer_simple_until_c, deserializer_simple_until_c_size>(
		[]<template<typename> typename Deserializer> {
			auto obj = std::get<simple>(serialized_values);
			obj.d = {};
			test_deserialization<Deserializer>(
				serialized_representations[5].substr(0, size_simple_until_c),
				serialized_representations_reversed[5].substr(0, size_simple_until_c),
				obj);
		}
	);
}

TEST(PackedSerializationTests, DeserializeUntilFieldTest2)
{
	run_for_types<deserializer_nested_until_b, deserializer_nested_until_b_size>(
		[]<template<typename> typename Deserializer> {
			auto obj = std::get<nested_short>(serialized_values);
			std::fill(&obj.c[0][0], &obj.c[0][0]
				+ sizeof(obj.c) / sizeof(obj.c[0][0]), simple{});
			std::fill(obj.d.begin(), obj.d.end(), 0u);
			test_deserialization<Deserializer>(
				serialized_representations[6].substr(0, size_nested_until_b),
				serialized_representations_reversed[6].substr(0, size_nested_until_b),
				obj);
		}
	);
}

namespace
{
constexpr auto size_b_partial
	= packed_reflection::get_type_size<decltype(nested_short::a)>()
	+ packed_reflection::get_type_size<decltype(nested_short::b)>()
	+ sizeof(std::uint16_t);

template<typename T>
using serializer_nested_b_partial = serialize_until_size_serializer<size_b_partial, T>;
template<typename T>
using serializer_3bytes = serialize_until_size_serializer<3u, T>;

constexpr auto size_b_partial2
	= packed_reflection::get_type_size<decltype(nested_short::a)>()
	+ packed_reflection::get_type_size<decltype(nested_short::b)>()
	+ sizeof(std::uint8_t) /* first field of simple struct */
	+ sizeof(std::uint32_t); /* second field of simple struct */
template<typename T>
using deserializer_nested_b_partial = deserialize_until_size_deserializer<size_b_partial2, T>;
} //namespace

TEST(PackedSerializationTests, SerializeUntilSizePartialTest1)
{
	test_serialization<serializer_nested_b_partial>(
		std::get<nested_short>(serialized_values),
		serialized_representations[6].substr(0, size_b_partial),
		serialized_representations_reversed[6].substr(0, size_b_partial));
}

TEST(PackedSerializationTests, SerializeUntilSizePartialTest2)
{
	test_serialization<serializer_3bytes>(
		std::get<std::uint64_t>(serialized_values),
		serialized_representations[4].substr(0, 3u),
		serialized_representations_reversed[4].substr(0, 3u));
}

TEST(PackedSerializationTests, DeserializeUntilSizePartialTest)
{
	auto obj = std::get<nested_short>(serialized_values);
	auto backup = obj.c[0][0];
	std::fill(&obj.c[0][0], &obj.c[0][0]
		+ sizeof(obj.c) / sizeof(obj.c[0][0]), simple{});
	obj.c[0][0].a = backup.a;
	obj.c[0][0].b = backup.b;
	std::fill(obj.d.begin(), obj.d.end(), 0u);
	test_deserialization<deserializer_nested_b_partial>(
		serialized_representations[6].substr(0, size_b_partial2),
		serialized_representations_reversed[6].substr(0, size_b_partial2),
		obj);
}
