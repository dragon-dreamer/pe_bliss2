#include "gtest/gtest.h"

#include <array>
#include <cstddef>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_container_buffer.h"
#include "buffers/input_memory_buffer.h"
#include "pe_bliss2/resources/accelerator_table.h"
#include "pe_bliss2/resources/accelerator_table_reader.h"
#include "pe_bliss2/resources/resource_reader_errc.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::resources;

TEST(AcceleratorTableReaderTests, ReadEmpty)
{
	buffers::input_container_buffer buf;
	buffers::input_buffer_stateful_wrapper_ref ref(buf);
	auto result = accelerator_table_from_resource(ref);
	EXPECT_TRUE(result.get_accelerators().empty());
	expect_contains_errors(result, resource_reader_errc::buffer_read_error);
}

namespace
{
constexpr std::size_t accelerator_count = 3u;
constexpr std::array accelerator_message{
	123u, 100u, 5u
};
constexpr std::array accelerator_modifiers{
	0x10u, 0x20u, 0x80u | 0x10u
};
constexpr std::array accelerator_key_codes{
	1u, 2u, 3u
};
constexpr std::array accelerators{
	std::byte{accelerator_modifiers[0]}, std::byte{}, //modifier
	std::byte{accelerator_key_codes[0]}, std::byte{}, //key_code
	std::byte{accelerator_message[0]}, std::byte{}, std::byte{}, std::byte{}, //message
	std::byte{accelerator_modifiers[1]}, std::byte{}, //modifier
	std::byte{accelerator_key_codes[1]}, std::byte{}, //key_code
	std::byte{accelerator_message[1]}, std::byte{}, std::byte{}, std::byte{}, //message
	std::byte{accelerator_modifiers[2]}, std::byte{}, //modifier
	std::byte{accelerator_key_codes[2]}, std::byte{}, //key_code
	std::byte{accelerator_message[2]}, std::byte{}, std::byte{}, std::byte{}, //message
};
} //namespace

TEST(AcceleratorTableReaderTests, ReadValid)
{
	buffers::input_memory_buffer buf(accelerators.data(), accelerators.size());
	buffers::input_buffer_stateful_wrapper_ref ref(buf);
	auto result = accelerator_table_from_resource(ref);
	ASSERT_EQ(result.get_accelerators().size(), accelerator_count);
	expect_contains_errors(result);
	for (std::size_t i = 0; i != accelerator_count; ++i)
	{
		const auto& accel = result.get_accelerators()[i].get_descriptor();
		EXPECT_EQ(accel->key_code, accelerator_key_codes[i]);
		EXPECT_EQ(accel->message, accelerator_message[i]);
		EXPECT_EQ(accel->modifier, accelerator_modifiers[i]);
	}
}

TEST(AcceleratorTableReaderTests, ReadLimit)
{
	buffers::input_memory_buffer buf(accelerators.data(), accelerators.size());
	buffers::input_buffer_stateful_wrapper_ref ref(buf);
	auto result = accelerator_table_from_resource(ref, { .max_accelerator_count = 1u });
	ASSERT_EQ(result.get_accelerators().size(), 1u);
	expect_contains_errors(result, accelerator_table_reader_errc::too_many_accelerators);
	const auto& accel = result.get_accelerators()[0].get_descriptor();
	EXPECT_EQ(accel->key_code, accelerator_key_codes[0]);
	EXPECT_EQ(accel->message, accelerator_message[0]);
	EXPECT_EQ(accel->modifier, accelerator_modifiers[0]);
}
