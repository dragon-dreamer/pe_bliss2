#include <algorithm>
#include <cstddef>
#include <memory>
#include <system_error>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "buffers/input_buffer_state.h"
#include "buffers/input_memory_buffer.h"
#include "buffers/input_virtual_buffer.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/output_memory_ref_buffer.h"

#include "pe_bliss2/packed_byte_vector.h"

#include "tests/tests/pe_bliss2/pe_error_helper.h"

#include "utilities/generic_error.h"

namespace
{
static constexpr std::size_t size = 10u;
auto create_test_vector()
{
	pe_bliss::packed_byte_vector vec;
	vec.value().resize(size - 2u);
	vec[0] = std::byte{ 123 };
	vec[5] = std::byte{ 100 };
	vec[7] = std::byte{ 5 };
	return vec;
}
} //namespace

TEST(PackedByteVectorTests, ConstructionAndMetadataTest)
{
	pe_bliss::packed_byte_vector vec;
	EXPECT_EQ(vec.value(), pe_bliss::packed_byte_vector::vector_type{});
	EXPECT_EQ(vec.get_state(), buffers::serialized_data_state{});
	EXPECT_EQ(vec.physical_size(), 0u);
	EXPECT_EQ(vec.data_size(), 0u);
	EXPECT_FALSE(vec.is_virtual());

	vec.value().resize(size);
	EXPECT_EQ(vec.physical_size(), size);
	EXPECT_EQ(vec.data_size(), size);
	EXPECT_FALSE(vec.is_virtual());

	vec.set_data_size(size - 2u);
	EXPECT_EQ(vec.physical_size(), size);
	EXPECT_EQ(vec.data_size(), size);
	EXPECT_FALSE(vec.is_virtual());

	vec.set_data_size(size + 2u);
	EXPECT_EQ(vec.physical_size(), size);
	EXPECT_EQ(vec.data_size(), size + 2u);
	EXPECT_TRUE(vec.is_virtual());

	buffers::serialized_data_state state;
	state.set_absolute_offset(1u);
	state.set_relative_offset(2u);
	state.set_buffer_pos(3u);
	vec[1] = std::byte{ 123 };
	vec.get_state() = state;

	auto copy1 = vec;
	EXPECT_EQ(copy1.value(), vec.value());
	EXPECT_EQ(copy1.get_state(), state);
	EXPECT_EQ(copy1.physical_size(), vec.physical_size());
	EXPECT_EQ(copy1.data_size(), vec.data_size());

	decltype(vec) copy2;
	copy2 = copy1;
	EXPECT_EQ(copy2.value(), vec.value());
	EXPECT_EQ(copy2.get_state(), state);
	EXPECT_EQ(copy2.physical_size(), vec.physical_size());
	EXPECT_EQ(copy2.data_size(), vec.data_size());
	EXPECT_EQ(copy2[1], std::byte{ 123 });
	EXPECT_EQ(std::as_const(copy2)[1], std::byte{ 123 });
}

TEST(PackedByteVectorTests, SerializeTest1)
{
	auto vec = create_test_vector();

	std::vector<std::byte> serialized(size, std::byte{ 1 });
	buffers::output_memory_ref_buffer buffer(serialized.data(), size);
	ASSERT_EQ(vec.serialize(buffer, false), vec.physical_size());
	EXPECT_TRUE(std::equal(vec.value().cbegin(), vec.value().cbegin()
		+ vec.physical_size(), serialized.cbegin()));
	EXPECT_EQ(serialized[vec.physical_size()], std::byte{ 1 });
	EXPECT_EQ(serialized[vec.physical_size() + 1], std::byte{ 1 });

	std::fill(serialized.begin(), serialized.end(), std::byte{ 1 });
	buffer.set_wpos(0u);
	vec.set_data_size(size);
	EXPECT_EQ(vec.data_size(), size);
	ASSERT_EQ(vec.serialize(buffer, true), vec.data_size());
	EXPECT_TRUE(std::equal(vec.value().cbegin(), vec.value().cbegin()
		+ vec.physical_size(), serialized.cbegin()));
	EXPECT_EQ(serialized[vec.physical_size()], std::byte{});
	EXPECT_EQ(serialized[vec.physical_size() + 1], std::byte{});
}

TEST(PackedByteVectorTests, SerializeTest2)
{
	auto vec = create_test_vector();

	std::vector<std::byte> serialized(size, std::byte{ 1 });

	expect_throw_pe_error([&] {
		vec.serialize(serialized.data(), vec.physical_size() - 1u, false); },
		utilities::generic_errc::buffer_overrun);
	ASSERT_EQ(vec.serialize(serialized.data(), vec.physical_size(), false),
		vec.physical_size());
	EXPECT_TRUE(std::equal(vec.value().cbegin(), vec.value().cbegin()
		+ vec.physical_size(), serialized.cbegin()));

	std::fill(serialized.begin(), serialized.end(), std::byte{ 1 });
	vec.set_data_size(size);
	expect_throw_pe_error([&] {
		vec.serialize(serialized.data(), vec.physical_size(), true); },
		utilities::generic_errc::buffer_overrun);
	ASSERT_EQ(vec.serialize(serialized.data(), vec.data_size(), true),
		vec.data_size());
	EXPECT_TRUE(std::equal(vec.value().cbegin(), vec.value().cbegin()
		+ vec.physical_size(), serialized.cbegin()));
	EXPECT_EQ(serialized[vec.physical_size()], std::byte{});
	EXPECT_EQ(serialized[vec.physical_size() + 1], std::byte{});
}

TEST(PackedByteVectorTests, DeserializeTest)
{
	pe_bliss::packed_byte_vector vec;

	static constexpr std::size_t absolute_offset = 1u;
	static constexpr std::size_t relative_offset = 2u;
	static constexpr std::size_t buffer_pos = 3u;
	
	const std::vector test_vector{
		std::byte{}, std::byte{}, std::byte{},
		std::byte{1}, std::byte{2}, std::byte{3},
		std::byte{10}, std::byte{20}, std::byte{30},
		std::byte{11}, std::byte{22}, std::byte{33},
		std::byte{50}
	};

	auto buffer = std::make_shared<buffers::input_memory_buffer>(
		test_vector.data(), test_vector.size());
	static constexpr std::size_t extra_virtual_bytes = size;
	buffers::input_virtual_buffer virtual_buffer(buffer, extra_virtual_bytes);
	buffers::input_buffer_stateful_wrapper_ref ref(virtual_buffer);

	{
		ref.set_rpos(buffer_pos);
		virtual_buffer.set_absolute_offset(absolute_offset);
		virtual_buffer.set_relative_offset(relative_offset);

		expect_throw_pe_error([&] {
			vec.deserialize(ref, size + 1, false); },
			utilities::generic_errc::buffer_overrun);

		ref.set_rpos(size);
		expect_throw_pe_error([&] {
			vec.deserialize(ref, size, false); },
			utilities::generic_errc::buffer_overrun);

		ref.set_rpos(ref.size());
		EXPECT_THROW(vec.deserialize(ref, 1u, false), std::system_error);

		ref.set_rpos(buffer_pos);
		ASSERT_NO_THROW(vec.deserialize(ref, size, false));

		EXPECT_EQ(vec.get_state().absolute_offset(), absolute_offset + buffer_pos);
		EXPECT_EQ(vec.get_state().relative_offset(), relative_offset + buffer_pos);
		EXPECT_EQ(vec.get_state().buffer_pos(), buffer_pos);
		EXPECT_FALSE(vec.is_virtual());
		ASSERT_EQ(vec.physical_size(), size);
		ASSERT_EQ(vec.data_size(), size);
		EXPECT_TRUE(std::equal(vec.value().cbegin(),
			vec.value().cbegin() + vec.physical_size(),
			test_vector.cbegin() + buffer_pos));
	}

	{
		ref.set_rpos(size);
		virtual_buffer.set_absolute_offset(0u);
		virtual_buffer.set_relative_offset(0u);
		ASSERT_NO_THROW(vec.deserialize(ref, size, true));
		EXPECT_EQ(vec.get_state().absolute_offset(), size);
		EXPECT_EQ(vec.get_state().relative_offset(), size);
		EXPECT_EQ(vec.get_state().buffer_pos(), size);
		EXPECT_TRUE(vec.is_virtual());
		ASSERT_EQ(vec.physical_size(), buffer_pos);
		ASSERT_EQ(vec.data_size(), size);
		EXPECT_TRUE(std::equal(vec.value().cbegin(),
			vec.value().cbegin() + vec.physical_size(),
			test_vector.cbegin() + size));
	}
}
