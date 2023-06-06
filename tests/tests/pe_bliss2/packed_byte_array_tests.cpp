#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <system_error>
#include <utility>

#include "gtest/gtest.h"

#include "buffers/input_buffer_state.h"
#include "buffers/input_memory_buffer.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_virtual_buffer.h"
#include "buffers/output_memory_ref_buffer.h"
#include "pe_bliss2/packed_byte_array.h"

#include "tests/pe_bliss2/pe_error_helper.h"

#include "utilities/generic_error.h"

namespace
{
static constexpr std::size_t size = 10u;
auto create_test_array()
{
	pe_bliss::packed_byte_array<size> arr;
	arr.set_physical_size(size - 2u);
	arr[0] = std::byte{ 123 };
	arr[5] = std::byte{ 100 };
	arr[7] = std::byte{ 5 };
	return arr;
}
} //namespace

TEST(PackedByteArrayTests, ConstructionAndMetadataTest)
{
	pe_bliss::packed_byte_array<size> arr;
	EXPECT_EQ(arr.max_size, size);
	EXPECT_EQ(arr.value(), (std::array<std::byte, size>{}));
	EXPECT_EQ(arr.get_state(), buffers::serialized_data_state{});
	EXPECT_EQ(arr.physical_size(), 0u);
	EXPECT_EQ(arr.data_size(), 0u);
	EXPECT_FALSE(arr.is_virtual());

	arr.set_physical_size(size + 10u);
	EXPECT_EQ(arr.physical_size(), size);
	EXPECT_EQ(arr.data_size(), size);
	EXPECT_FALSE(arr.is_virtual());

	arr.set_physical_size(size - 1u);
	EXPECT_EQ(arr.physical_size(), size - 1u);
	EXPECT_EQ(arr.data_size(), size);
	EXPECT_TRUE(arr.is_virtual());

	arr.set_data_size(size - 2u);
	EXPECT_EQ(arr.physical_size(), size - 1u);
	EXPECT_EQ(arr.data_size(), size - 1u);

	buffers::serialized_data_state state;
	state.set_absolute_offset(1u);
	state.set_relative_offset(2u);
	state.set_buffer_pos(3u);
	arr[1] = std::byte{ 123 };
	arr.get_state() = state;
	
	auto copy1 = arr;
	EXPECT_EQ(copy1.value(), arr.value());
	EXPECT_EQ(copy1.get_state(), state);
	EXPECT_EQ(copy1.physical_size(), arr.physical_size());
	EXPECT_EQ(copy1.data_size(), arr.data_size());

	decltype(arr) copy2;
	copy2 = copy1;
	EXPECT_EQ(copy2.value(), arr.value());
	EXPECT_EQ(copy2.get_state(), state);
	EXPECT_EQ(copy2.physical_size(), arr.physical_size());
	EXPECT_EQ(copy2.data_size(), arr.data_size());
	EXPECT_EQ(copy2[1], std::byte{ 123 });
	EXPECT_EQ(std::as_const(copy2)[1], std::byte{ 123 });

	pe_bliss::packed_byte_array<size + 10u> arr2;
	arr2.copy_metadata_from(arr);
	EXPECT_EQ(arr2.get_state(), arr.get_state());
	EXPECT_EQ(arr2.physical_size(), arr.physical_size());
	EXPECT_EQ(arr2.data_size(), arr.data_size());
}

TEST(PackedByteArrayTests, SerializeTest1)
{
	auto arr = create_test_array();

	std::array<std::byte, size> serialized{};
	buffers::output_memory_ref_buffer buffer(serialized.data(), size);
	ASSERT_EQ(arr.serialize(buffer, false), arr.physical_size());
	EXPECT_EQ(arr.value(), serialized);
	
	serialized = {};
	buffer.set_wpos(0u);
	arr.set_physical_size(size - 4u);
	ASSERT_EQ(arr.serialize(buffer, false), arr.physical_size());
	EXPECT_EQ(std::memcmp(serialized.data(), arr.value().data(),
		arr.physical_size()), 0);
	EXPECT_EQ(serialized[7], std::byte{});

	serialized = {};
	buffer.set_wpos(0u);
	EXPECT_EQ(arr.data_size(), size - 2u);
	EXPECT_EQ(arr.serialize(buffer, true), arr.data_size());
	EXPECT_EQ(arr.value(), serialized);
}

TEST(PackedByteArrayTests, SerializeTest2)
{
	auto arr = create_test_array();

	std::array<std::byte, size> serialized{};

	expect_throw_pe_error([&] {
		arr.serialize(serialized.data(), arr.physical_size() - 1u, false); },
		utilities::generic_errc::buffer_overrun);
	ASSERT_EQ(arr.serialize(serialized.data(), arr.physical_size(), false),
		arr.physical_size());
	EXPECT_EQ(arr.value(), serialized);

	serialized = {};
	arr.set_physical_size(size - 4u);
	ASSERT_EQ(arr.serialize(serialized.data(), arr.physical_size(), false),
		arr.physical_size());
	EXPECT_EQ(std::memcmp(serialized.data(), arr.value().data(),
		arr.physical_size()), 0);

	serialized = {};
	expect_throw_pe_error([&] {
		arr.serialize(serialized.data(), arr.physical_size(), true); },
		utilities::generic_errc::buffer_overrun);
	ASSERT_EQ(arr.serialize(serialized.data(), arr.data_size(), true),
		arr.data_size());
	EXPECT_EQ(arr.value(), serialized);
}

TEST(PackedByteArrayTests, DeserializeTest)
{
	pe_bliss::packed_byte_array<size> arr;

	static constexpr std::size_t absolute_offset = 1u;
	static constexpr std::size_t relative_offset = 2u;
	static constexpr std::size_t buffer_pos = 3u;

	static constexpr std::array<std::byte, size + buffer_pos> test_array{
		std::byte{}, std::byte{}, std::byte{},
		std::byte{1}, std::byte{2}, std::byte{3},
		std::byte{10}, std::byte{20}, std::byte{30},
		std::byte{11}, std::byte{22}, std::byte{33},
		std::byte{50}
	};

	auto buffer = std::make_shared<buffers::input_memory_buffer>(
		test_array.data(), test_array.size());
	static constexpr std::size_t extra_virtual_bytes = size;
	buffers::input_virtual_buffer virtual_buffer(buffer, extra_virtual_bytes);
	buffers::input_buffer_stateful_wrapper_ref ref(virtual_buffer);

	{
		ref.set_rpos(buffer_pos);
		virtual_buffer.set_absolute_offset(absolute_offset);
		virtual_buffer.set_relative_offset(relative_offset);

		expect_throw_pe_error([&] {
			arr.deserialize(ref, size + 1, false); },
			utilities::generic_errc::buffer_overrun);

		ref.set_rpos(size);
		expect_throw_pe_error([&] {
			arr.deserialize(ref, extra_virtual_bytes, false); },
			utilities::generic_errc::buffer_overrun);

		ref.set_rpos(ref.size());
		EXPECT_THROW(arr.deserialize(ref, 1u, false), std::system_error);

		ref.set_rpos(buffer_pos);
		ASSERT_NO_THROW(arr.deserialize(ref, size, false));

		EXPECT_EQ(arr.get_state().absolute_offset(), absolute_offset + buffer_pos);
		EXPECT_EQ(arr.get_state().relative_offset(), relative_offset + buffer_pos);
		EXPECT_EQ(arr.get_state().buffer_pos(), buffer_pos);
		EXPECT_FALSE(arr.is_virtual());
		EXPECT_EQ(arr.physical_size(), size);
		EXPECT_EQ(arr.data_size(), size);
		EXPECT_TRUE(std::equal(arr.value().cbegin(),
			arr.value().cbegin() + arr.physical_size(),
			test_array.cbegin() + buffer_pos));
	}

	{
		virtual_buffer.set_absolute_offset(0u);
		virtual_buffer.set_relative_offset(0u);
		ref.set_rpos(size);
		ASSERT_NO_THROW(arr.deserialize(ref, extra_virtual_bytes, true));
		EXPECT_EQ(arr.get_state().absolute_offset(), size);
		EXPECT_EQ(arr.get_state().relative_offset(), size);
		EXPECT_EQ(arr.get_state().buffer_pos(), size);
		EXPECT_TRUE(arr.is_virtual());
		EXPECT_EQ(arr.physical_size(), buffer_pos);
		EXPECT_EQ(arr.data_size(), size);
		EXPECT_TRUE(std::equal(arr.value().cbegin(),
			arr.value().cbegin() + arr.physical_size(),
			test_array.cbegin() + size));
	}
}
