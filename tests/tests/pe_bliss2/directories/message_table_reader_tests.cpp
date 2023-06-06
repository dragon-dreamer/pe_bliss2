#include <array>
#include <cstddef>
#include <memory>
#include <utility>
#include <variant>

#include "gtest/gtest.h"

#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_container_buffer.h"
#include "buffers/input_memory_buffer.h"
#include "buffers/input_virtual_buffer.h"
#include "pe_bliss2/resources/message_table.h"
#include "pe_bliss2/resources/message_table_reader.h"
#include "pe_bliss2/resources/resource_reader_errc.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::resources;

namespace
{

constexpr std::uint32_t number_of_blocks = 3u;
constexpr std::uint32_t block1_low_id = 5u;
constexpr std::uint32_t block1_high_id = 6u;
constexpr std::uint32_t block2_low_id = 6u;
constexpr std::uint32_t block2_high_id = 7u;
constexpr std::uint32_t block3_low_id = 10u;
constexpr std::uint32_t block3_high_id = 9u;

constexpr std::uint32_t block1_offset_to_entries = 40u;
constexpr std::uint32_t block2_offset_to_entries = block1_offset_to_entries + 16u;

constexpr std::array table_data{
	//number_of_blocks
	std::byte{number_of_blocks}, std::byte{}, std::byte{}, std::byte{},

	//block 1
	std::byte{block1_low_id}, std::byte{}, std::byte{}, std::byte{}, //low_id
	std::byte{block1_high_id}, std::byte{}, std::byte{}, std::byte{}, //high_id
	//offset_to_entries
	std::byte{block1_offset_to_entries}, std::byte{}, std::byte{}, std::byte{},

	//block 2
	std::byte{block2_low_id}, std::byte{}, std::byte{}, std::byte{}, //low_id
	std::byte{block2_high_id}, std::byte{}, std::byte{}, std::byte{}, //high_id
	//offset_to_entries
	std::byte{block2_offset_to_entries}, std::byte{}, std::byte{}, std::byte{},

	//block 3
	std::byte{block3_low_id}, std::byte{}, std::byte{}, std::byte{}, //low_id
	std::byte{block3_high_id}, std::byte{}, std::byte{}, std::byte{}, //high_id
	std::byte{}, std::byte{}, std::byte{}, std::byte{}, //offset_to_entries

	//block 1 entries
	std::byte{8}, std::byte{}, //length
	std::byte{}, std::byte{}, //flags - ansi
	std::byte{'a'}, std::byte{'b'}, std::byte{'c'}, std::byte{}, //text
	std::byte{8}, std::byte{}, //length
	std::byte{2}, std::byte{}, //flags - utf8
	std::byte{'x'}, std::byte{'y'}, std::byte{'z'}, std::byte{}, //text

	//block 2 entries
	std::byte{5}, std::byte{}, //length
	std::byte{3}, std::byte{}, //flags - invalid
	std::byte{},
	std::byte{12}, std::byte{}, //length
	std::byte{1}, std::byte{}, //flags - unicode
	//text
	std::byte{'a'}, std::byte{},
	std::byte{'b'}, std::byte{},
	std::byte{'c'}, std::byte{},
	std::byte{}, std::byte{},
};

void validate_block1(const message_table_details& table,
	std::size_t message_count = block1_high_id - block1_low_id + 1u)
{
	const auto& block1 = table.get_message_blocks()[0];
	expect_contains_errors(block1);
	EXPECT_EQ(block1.get_start_id(), block1_low_id);
	ASSERT_EQ(block1.get_entries().size(), message_count);
	const auto& block1_entry1 = block1.get_entries()[0];
	expect_contains_errors(block1_entry1);
	const auto* message1 = std::get_if<ansi_message>(&block1_entry1.get_message());
	ASSERT_NE(message1, nullptr);
	EXPECT_EQ(message1->value(), "abc");
	if (message_count > 1u)
	{
		const auto& block1_entry2 = block1.get_entries()[1];
		expect_contains_errors(block1_entry2);
		const auto* message2 = std::get_if<utf8_message>(&block1_entry2.get_message());
		ASSERT_NE(message2, nullptr);
		EXPECT_EQ(message2->value(), u8"xyz");
	}
}

void validate_block2(const message_table_details& table, bool is_virtual_string = false)
{
	const auto& block2 = table.get_message_blocks()[1];
	expect_contains_errors(block2);
	EXPECT_EQ(block2.get_start_id(), block2_low_id);
	ASSERT_EQ(block2.get_entries().size(), block2_high_id - block2_low_id + 1u);
	const auto& block2_entry1 = block2.get_entries()[0];
	expect_contains_errors(block2_entry1,
		message_table_reader_errc::invalid_message_block_encoding);
	EXPECT_TRUE(std::holds_alternative<std::monostate>(block2_entry1.get_message()));
	const auto& block2_entry2 = block2.get_entries()[1];
	if (is_virtual_string)
		expect_contains_errors(block2_entry2, message_table_reader_errc::virtual_message_memory);
	else
		expect_contains_errors(block2_entry2);
	const auto* message3 = std::get_if<unicode_message>(&block2_entry2.get_message());
	ASSERT_NE(message3, nullptr);
	EXPECT_EQ(message3->value(), u"abc");
}

void validate_block3(const message_table_details& table)
{
	const auto& block3 = table.get_message_blocks()[2];
	expect_contains_errors(block3, message_table_reader_errc::invalid_low_high_ids);
	EXPECT_TRUE(block3.get_entries().empty());
}

} //namespace

TEST(MessageTableReaderTests, ReadEmpty)
{
	buffers::input_container_buffer buf;
	buffers::input_buffer_stateful_wrapper_ref ref(buf);
	auto table = message_table_from_resource(ref);
	expect_contains_errors(table, resource_reader_errc::buffer_read_error);
	EXPECT_TRUE(table.get_message_blocks().empty());
}

TEST(MessageTableReaderTests, ReadValid)
{
	buffers::input_memory_buffer buf(table_data.data(), table_data.size());
	buffers::input_buffer_stateful_wrapper_ref ref(buf);
	auto table = message_table_from_resource(ref);
	expect_contains_errors(table,
		message_table_reader_errc::overlapping_message_ids);
	ASSERT_EQ(table.get_message_blocks().size(), number_of_blocks);

	validate_block1(table);
	validate_block2(table);
	validate_block3(table);
}

TEST(MessageTableReaderTests, ReadValidBlockLimit)
{
	buffers::input_memory_buffer buf(table_data.data(), table_data.size());
	buffers::input_buffer_stateful_wrapper_ref ref(buf);
	auto table = message_table_from_resource(ref, { .max_block_count = 1u });
	expect_contains_errors(table, message_table_reader_errc::too_many_blocks);
	ASSERT_EQ(table.get_message_blocks().size(), 1u);
	validate_block1(table);
}

TEST(MessageTableReaderTests, ReadValidMessageLimit)
{
	buffers::input_memory_buffer buf(table_data.data(), table_data.size());
	buffers::input_buffer_stateful_wrapper_ref ref(buf);
	auto table = message_table_from_resource(ref, { .max_message_count = 1u });
	expect_contains_errors(table, message_table_reader_errc::too_many_messages);
	ASSERT_EQ(table.get_message_blocks().size(), 1u);
	validate_block1(table, 1u);
}

TEST(MessageTableReaderTests, ReadValidVirtualError)
{
	auto buf = std::make_shared<buffers::input_memory_buffer>(table_data.data(),
		table_data.size() - 2u); //last string bytes are virtual
	buffers::input_virtual_buffer vbuf(std::move(buf), 2u);
	buffers::input_buffer_stateful_wrapper_ref ref(vbuf);
	auto table = message_table_from_resource(ref);
	ASSERT_EQ(table.get_message_blocks().size(), number_of_blocks);
	validate_block1(table);
	validate_block2(table, true);
	validate_block3(table);
}

TEST(MessageTableReaderTests, ReadValidVirtual)
{
	auto buf = std::make_shared<buffers::input_memory_buffer>(table_data.data(),
		table_data.size() - 2u); //last string bytes are virtual
	buffers::input_virtual_buffer vbuf(std::move(buf), 2u);
	buffers::input_buffer_stateful_wrapper_ref ref(vbuf);
	auto table = message_table_from_resource(ref, { .allow_virtual_data = true });
	ASSERT_EQ(table.get_message_blocks().size(), number_of_blocks);
	validate_block1(table);
	validate_block2(table);
	validate_block3(table);
}
