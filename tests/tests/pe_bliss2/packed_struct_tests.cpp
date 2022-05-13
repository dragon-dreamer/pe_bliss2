#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

#include <boost/endian.hpp>

#include "gtest/gtest.h"

#include "buffers/input_memory_buffer.h"
#include "buffers/output_memory_buffer.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/detail/packed_serialization.h"

#include "tests/tests/pe_bliss2/pe_error_helper.h"
#include "tests/tests/pe_bliss2/test_structs.h"

#include "utilities/generic_error.h"

namespace
{

nested create_test_struct()
{
	nested obj{};
	obj.b.c = 123;
	obj.e[2][1].d = 56789;
	obj.f[4] = 0xabc;
	return obj;
}

constexpr auto total_size
	= pe_bliss::detail::packed_reflection::get_type_size<nested>();

} //namespace

TEST(PackedStructTests, ConstructionTest)
{
	auto obj = create_test_struct();

	{
		pe_bliss::packed_struct wrapped(obj);
		EXPECT_EQ(wrapped.get(), obj);
		EXPECT_EQ(*wrapped.operator->(), obj);

		pe_bliss::packed_struct copy(obj);
		EXPECT_EQ(copy.get(), obj);
		EXPECT_EQ(*copy.operator->(), obj);
	}

	{
		pe_bliss::packed_struct<nested> wrapped;
		EXPECT_EQ(wrapped.get(), nested{});
		EXPECT_EQ(*wrapped.operator->(), nested{});

		wrapped = obj;
		EXPECT_EQ(wrapped.get(), obj);
		EXPECT_EQ(*wrapped.operator->(), obj);

		pe_bliss::packed_struct<nested> copy;
		copy = wrapped;
		EXPECT_EQ(copy.get(), obj);
		EXPECT_EQ(*copy.operator->(), obj);
	}
}

TEST(PackedStructTests, OffsetsSizesTest)
{
	pe_bliss::packed_struct<nested> wrapped;
	EXPECT_EQ(wrapped.get_state(), buffers::serialized_data_state{});
	EXPECT_EQ(wrapped.physical_size(), total_size);
	EXPECT_FALSE(wrapped.is_virtual());

	wrapped.get_state().set_absolute_offset(1);
	wrapped.get_state().set_relative_offset(2);
	wrapped.get_state().set_buffer_pos(3);
	EXPECT_EQ(wrapped.get_state().absolute_offset(), 1u);
	EXPECT_EQ(wrapped.get_state().relative_offset(), 2u);
	EXPECT_EQ(wrapped.get_state().buffer_pos(), 3u);

	wrapped.set_physical_size(total_size + 100);
	EXPECT_EQ(wrapped.physical_size(), total_size);
	EXPECT_FALSE(wrapped.is_virtual());

	wrapped.set_physical_size(total_size / 2);
	EXPECT_EQ(wrapped.physical_size(), total_size / 2);
	EXPECT_TRUE(wrapped.is_virtual());

	pe_bliss::packed_struct<nested> copy;
	copy.copy_metadata_from(wrapped);
	EXPECT_EQ(copy.get_state().absolute_offset(), 1u);
	EXPECT_EQ(copy.get_state().relative_offset(), 2u);
	EXPECT_EQ(copy.get_state().buffer_pos(), 3u);
	EXPECT_EQ(copy.physical_size(), total_size / 2);
	EXPECT_TRUE(copy.is_virtual());
}

namespace
{
template<typename T>
class PackedStructTestsWithEndianness : public testing::Test
{
public:
	static constexpr auto endianness = T::value;
};
} //namespace

using tested_types = ::testing::Types<
	std::integral_constant<boost::endian::order, boost::endian::order::little>,
	std::integral_constant<boost::endian::order, boost::endian::order::big>>;

TYPED_TEST_SUITE(PackedStructTestsWithEndianness, tested_types);

TYPED_TEST(PackedStructTestsWithEndianness, SerializeTest)
{
	pe_bliss::packed_struct<nested, TestFixture::endianness> wrapped(
		create_test_struct());
	std::array<std::byte, total_size> result;
	pe_bliss::detail::packed_serialization<TestFixture::endianness>::serialize(
		wrapped.get(), result.data());
	ASSERT_EQ(wrapped.serialize(), result);

	{
		std::vector<std::byte> serialized;
		buffers::output_memory_buffer buffer(serialized);
		wrapped.serialize(buffer, false);
		ASSERT_EQ(serialized.size(), total_size);
		EXPECT_TRUE(std::equal(serialized.cbegin(), serialized.cend(), result.cbegin()));
	}

	{
		std::array<std::byte, total_size> serialized{};
		expect_throw_pe_error([&] {
			wrapped.serialize(serialized.data(), total_size / 2, false); },
			utilities::generic_errc::buffer_overrun);
		EXPECT_EQ(serialized, decltype(serialized){});
		ASSERT_EQ(wrapped.serialize(serialized.data(),
			total_size + 10u, false), total_size);
		EXPECT_EQ(serialized, result);
	}

	{
		std::array<std::byte, total_size / 2> serialized{};
		ASSERT_EQ(wrapped.serialize_until(serialized.data(), total_size / 2, false),
			total_size / 2);
		EXPECT_TRUE(std::equal(serialized.cbegin(), serialized.cend(), result.cbegin()));
	}
}

TYPED_TEST(PackedStructTestsWithEndianness, SerializeVirtualTest)
{
	pe_bliss::packed_struct<nested, TestFixture::endianness> wrapped(
		create_test_struct());
	static constexpr auto physical_size = total_size - 10u;
	wrapped.set_physical_size(physical_size);

	std::array<std::byte, total_size> result;
	pe_bliss::detail::packed_serialization<TestFixture::endianness>::serialize(
		wrapped.get(), result.data());
	ASSERT_EQ(wrapped.serialize(), result);

	{
		std::vector<std::byte> serialized;
		buffers::output_memory_buffer buffer(serialized);
		wrapped.serialize(buffer, true);
		ASSERT_EQ(serialized.size(), total_size);
		EXPECT_TRUE(std::equal(serialized.cbegin(), serialized.cend(), result.cbegin()));
	}

	{
		std::vector<std::byte> serialized;
		buffers::output_memory_buffer buffer(serialized);
		wrapped.serialize(buffer, false);
		ASSERT_EQ(serialized.size(), physical_size);
		EXPECT_TRUE(std::equal(serialized.cbegin(), serialized.cend(), result.cbegin()));
	}

	{
		std::array<std::byte, physical_size> serialized{};
		expect_throw_pe_error([&] {
			wrapped.serialize(serialized.data(), physical_size, true); },
			utilities::generic_errc::buffer_overrun);
		EXPECT_EQ(serialized, decltype(serialized){});
		ASSERT_EQ(wrapped.serialize(serialized.data(),
			physical_size, false), physical_size);
		EXPECT_TRUE(std::equal(serialized.cbegin(), serialized.cend(), result.cbegin()));
	}

	{
		std::array<std::byte, physical_size + 5u> serialized{};
		serialized[physical_size] = std::byte{ 0xff };
		ASSERT_EQ(wrapped.serialize_until(serialized.data(), serialized.size(), false),
			physical_size);
		EXPECT_TRUE(std::equal(serialized.cbegin(),
			serialized.cbegin() + physical_size, result.cbegin()));
		EXPECT_EQ(serialized[physical_size], std::byte{ 0xff });
	}
}

TYPED_TEST(PackedStructTestsWithEndianness, DeserializeTest)
{
	std::array<std::byte, total_size + 1> result;
	auto test_struct = create_test_struct();
	pe_bliss::detail::packed_serialization<TestFixture::endianness>::serialize(
		test_struct, result.data() + 1);

	buffers::input_memory_buffer buffer(result.data(), result.size());
	static constexpr std::size_t rpos = 1u;
	static constexpr std::size_t absolute_offset = 2u;
	static constexpr std::size_t relative_offset = 3u;
	buffer.set_rpos(rpos);
	buffer.set_absolute_offset(absolute_offset);
	buffer.set_relative_offset(relative_offset);

	pe_bliss::packed_struct<nested, TestFixture::endianness> wrapped;
	ASSERT_NO_THROW(wrapped.deserialize(buffer, false));
	ASSERT_EQ(wrapped.physical_size(), total_size);
	EXPECT_FALSE(wrapped.is_virtual());
	EXPECT_EQ(buffer.rpos(), rpos + total_size);
	EXPECT_EQ(wrapped.get(), test_struct);
	EXPECT_EQ(wrapped.get_state().absolute_offset(), rpos + absolute_offset);
	EXPECT_EQ(wrapped.get_state().relative_offset(), rpos + relative_offset);
	EXPECT_EQ(wrapped.get_state().buffer_pos(), rpos);

	buffer.set_rpos(rpos);
	wrapped.get() = {};
	ASSERT_NO_THROW(wrapped.deserialize_until(buffer, total_size / 2, false));
	ASSERT_EQ(wrapped.physical_size(), total_size / 2);
	EXPECT_EQ(buffer.rpos(), rpos + total_size / 2);
	EXPECT_TRUE(wrapped.is_virtual());

	decltype(test_struct) original{};
	pe_bliss::detail::packed_serialization<TestFixture::endianness>::deserialize_until(
		original, result.data() + 1, total_size / 2);
	EXPECT_EQ(wrapped.get(), original);

	buffers::input_memory_buffer truncated_buffer(result.data(), result.size() / 2);
	expect_throw_pe_error([&] {
		wrapped.deserialize(buffer, false); },
		utilities::generic_errc::buffer_overrun);
	EXPECT_EQ(wrapped.get(), original);
	EXPECT_EQ(wrapped.physical_size(), total_size / 2);

	buffer.set_rpos(buffer.size());
	expect_throw_pe_error([&] {
		wrapped.deserialize_until(buffer, total_size, false); },
		utilities::generic_errc::buffer_overrun);
	EXPECT_EQ(wrapped.get(), original);
	EXPECT_EQ(wrapped.physical_size(), total_size / 2);
}

TYPED_TEST(PackedStructTestsWithEndianness, DeserializeVirtualTest)
{
	std::array<std::byte, total_size / 2> result;
	auto test_struct = create_test_struct();
	pe_bliss::detail::packed_serialization<TestFixture::endianness>::serialize_until(
		test_struct, result.data(), result.size());
	decltype(test_struct) original{};
	pe_bliss::detail::packed_serialization<TestFixture::endianness>::deserialize_until(
		original, result.data(), result.size());

	pe_bliss::packed_struct<nested, TestFixture::endianness> wrapped;
	buffers::input_memory_buffer truncated_buffer(result.data(), result.size());
	expect_throw_pe_error([&] {
		wrapped.deserialize(truncated_buffer, false); },
		utilities::generic_errc::buffer_overrun);

	truncated_buffer.set_rpos(0u);
	ASSERT_NO_THROW(wrapped.deserialize(truncated_buffer, true));
	EXPECT_EQ(wrapped.physical_size(), result.size());
	EXPECT_EQ(wrapped.get(), original);
	EXPECT_TRUE(wrapped.is_virtual());
	EXPECT_EQ(truncated_buffer.rpos(), result.size());
}
