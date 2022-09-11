#pragma once

#include <system_error>
#include <type_traits>

#include "pe_bliss2/resources/message_table.h"

#include "buffers/input_buffer_stateful_wrapper.h"

namespace pe_bliss::resources
{

enum class message_table_reader_errc
{
	too_many_blocks = 1,
	too_many_messages,
	invalid_low_high_ids,
	invalid_message_block,
	invalid_message_block_encoding,
	invalid_message_length,
	virtual_message_memory,
	overlapping_message_ids,
	invalid_message
};

std::error_code make_error_code(message_table_reader_errc) noexcept;

struct [[nodiscard]] message_table_read_options
{
	bool allow_virtual_memory = false;
	std::uint32_t max_block_count = 0xfffu;
	std::uint32_t max_message_count = 0xffffu;
};

[[nodiscard]]
message_table_details message_table_from_resource(
	buffers::input_buffer_stateful_wrapper_ref& buf,
	const message_table_read_options& options = {});

} //namespace pe_bliss::resources

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::resources::message_table_reader_errc> : true_type {};
} //namespace std
