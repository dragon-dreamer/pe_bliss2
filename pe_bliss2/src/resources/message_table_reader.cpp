#include "pe_bliss2/resources/message_table_reader.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <set>
#include <string>
#include <system_error>

#include <boost/endian/conversion.hpp>

#include "buffers/input_buffer_interface.h"
#include "pe_bliss2/detail/resources/message_table.h"
#include "pe_bliss2/resources/resource_reader_errc.h"
#include "utilities/safe_uint.h"

namespace
{
struct message_table_reader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "message_table_reader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::resources::message_table_reader_errc;
		switch (static_cast<pe_bliss::resources::message_table_reader_errc>(ev))
		{
		case too_many_blocks:
			return "Too many message blocks";
		case too_many_messages:
			return "Too many message entries";
		case invalid_low_high_ids:
			return "Invalid message block high/low IDs";
		case invalid_message_block:
			return "Invalid message block";
		case invalid_message_block_encoding:
			return "Invalid message block encoding";
		case invalid_message_length:
			return "Invalid message length";
		case virtual_message_memory:
			return "Message has virtual bytes";
		case overlapping_message_ids:
			return "Overlapping message IDs";
		case invalid_message:
			return "Invalid message";
		default:
			return {};
		}
	}
};

const message_table_reader_error_category message_table_reader_error_category_instance;

struct interval
{
	std::uint32_t from{}, to{};

	[[nodiscard]]
	friend bool operator<(const interval& l, const interval& r) noexcept
	{
		return l.to < r.from;
	}
};

} // namespace

namespace pe_bliss::resources
{

std::error_code make_error_code(message_table_reader_errc e) noexcept
{
	return { static_cast<int>(e), message_table_reader_error_category_instance };
}

template<message_encoding Encoding>
void load_message(buffers::input_buffer_stateful_wrapper_ref& entry_buf,
	message_entry_details& entry,
	std::uint16_t length, message_string<Encoding>& message,
	bool allow_virtual_data)
{
	using char_type = typename message_string<Encoding>::string_type::value_type;
	static constexpr auto char_size = sizeof(char_type);

	if (length < message_entry_details::descriptor_type::packed_size + char_size)
	{
		entry.add_error(message_table_reader_errc::invalid_message_length);
		return;
	}

	const auto start_pos = entry_buf.rpos();

	length -= message_entry_details::descriptor_type::packed_size
		+ char_size /* trailing nullbyte */;
	if (length % char_size)
		entry.add_error(message_table_reader_errc::invalid_message_length);

	auto& value = message.value();
	value.resize(length / char_size);

	std::size_t total_physical_bytes_read = 0;
	std::size_t physical_bytes_read = 0;
	char_type ch{};
	std::size_t index = 0;
	auto string_length = value.size();
	while (string_length-- && (physical_bytes_read = entry_buf.read(char_size,
		reinterpret_cast<std::byte*>(&ch))) == char_size)
	{
		total_physical_bytes_read += physical_bytes_read;
		if constexpr (Encoding != message_encoding::utf8)
			boost::endian::little_to_native_inplace(ch);
		value[index++] = ch;
	}

	// read nullbyte
	total_physical_bytes_read += entry_buf.read(char_size,
		reinterpret_cast<std::byte*>(&ch));

	if (!allow_virtual_data
		&& total_physical_bytes_read != (message.value().size() + 1u) * char_size)
	{
		message.value().resize(total_physical_bytes_read / char_size);
		entry.add_error(message_table_reader_errc::virtual_message_memory);
	}

	entry_buf.set_rpos(start_pos + length + char_size);
}

void load_entries(const message_table_read_options& options,
	std::uint32_t total_entries, std::size_t start_rpos,
	message_block_details& block, buffers::input_buffer_interface& buf)
{
	buffers::input_buffer_stateful_wrapper_ref entry_buf(buf);
	try
	{
		utilities::safe_uint rpos = start_rpos;
		rpos += block.get_descriptor()->offset_to_entries;
		entry_buf.set_rpos(rpos.value());
	}
	catch (const std::system_error&)
	{
		block.add_error(message_table_reader_errc::invalid_message_block);
		return;
	}

	while (total_entries--)
	{
		auto& entry = block.get_entries().emplace_back();
		try
		{
			entry.get_descriptor().deserialize(entry_buf, options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			entry.add_error(resource_reader_errc::buffer_read_error);
			return;
		}

		try
		{
			auto length = entry.get_descriptor()->length;
			switch (entry.get_descriptor()->flags)
			{
			case detail::resources::message_resource_ansi:
				load_message(entry_buf, entry, length,
					entry.get_message().emplace<ansi_message>(),
					options.allow_virtual_data);
				break;
			case detail::resources::message_resource_unicode:
				load_message(entry_buf, entry, length,
					entry.get_message().emplace<unicode_message>(),
					options.allow_virtual_data);
				break;
			case detail::resources::message_resource_utf8:
				load_message(entry_buf, entry, length,
					entry.get_message().emplace<utf8_message>(),
					options.allow_virtual_data);
				break;
			default:
				entry.add_error(message_table_reader_errc::invalid_message_block_encoding);
				if (entry.get_descriptor()->length >
					message_entry_details::descriptor_type::packed_size)
				{
					entry_buf.advance_rpos(entry.get_descriptor()->length -
						message_entry_details::descriptor_type::packed_size);
				}
				break;
			}
		}
		catch (const std::system_error&)
		{
			entry.add_error(message_table_reader_errc::invalid_message);
			break;
		}
	}
}

void load_blocks(buffers::input_buffer_stateful_wrapper_ref& buf,
	const message_table_read_options& options,
	std::size_t start_rpos, std::uint32_t total_blocks,
	message_table_details& table)
{
	std::set<interval> used_ids;
	std::uint32_t remaining_entries = options.max_message_count;
	table.get_message_blocks().reserve(total_blocks);
	while (total_blocks--)
	{
		auto& block = table.get_message_blocks().emplace_back();
		try
		{
			block.get_descriptor().deserialize(buf, options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			block.add_error(resource_reader_errc::buffer_read_error);
			break;
		}

		const auto high_id = block.get_descriptor()->high_id;
		const auto low_id = block.get_descriptor()->low_id;
		if (low_id > high_id
			|| high_id - low_id == (std::numeric_limits<std::uint32_t>::max)())
		{
			block.add_error(message_table_reader_errc::invalid_low_high_ids);
			continue;
		}

		std::uint32_t total_entries = high_id - low_id + 1u;
		if (total_entries > remaining_entries)
		{
			total_entries = remaining_entries;
			remaining_entries = 0;
			table.add_error(message_table_reader_errc::too_many_messages);
		}
		else
		{
			remaining_entries -= total_entries;
		}

		if (!used_ids.insert({ .from = low_id, .to = high_id }).second)
			table.add_error(message_table_reader_errc::overlapping_message_ids);

		load_entries(options, total_entries,
			start_rpos, block, buf.get_buffer());
		if (!remaining_entries)
			break;
	}
}

message_table_details message_table_from_resource(
	buffers::input_buffer_stateful_wrapper_ref& buf,
	const message_table_read_options& options)
{
	message_table_details table;

	const auto start_rpos = buf.rpos();
	try
	{
		table.get_descriptor().deserialize(buf, options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		table.add_error(resource_reader_errc::buffer_read_error);
		return table;
	}

	auto total_blocks = table.get_descriptor()->number_of_blocks;
	if (total_blocks > options.max_block_count)
	{
		total_blocks = options.max_block_count;
		table.add_error(message_table_reader_errc::too_many_blocks);
	}
	
	load_blocks(buf, options, start_rpos, total_blocks, table);
	return table;
}

} //namespace pe_bliss::resources
