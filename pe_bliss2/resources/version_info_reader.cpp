#include "pe_bliss2/resources/version_info_reader.h"

#include <cstdint>
#include <string>
#include <system_error>

#include "buffers/input_buffer_section.h"
#include "pe_bliss2/pe_types.h"
#include "pe_bliss2/resources/resource_reader_errc.h"
#include "utilities/math.h"

namespace
{

struct version_info_reader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "version_info_reader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::resources::version_info_reader_errc;
		switch (static_cast<pe_bliss::resources::version_info_reader_errc>(ev))
		{
		case unaligned_version_info_block:
			return "Unaligned version info block";
		case invalid_value_length:
			return "Invalid value length";
		case value_read_error:
			return "Value read error";
		case invalid_block_length:
			return "Invalid block length";
		case key_read_error:
			return "Key read error";
		case excessive_data_in_buffer:
			return "Excessive data in the buffer";
		case child_read_error:
			return "Child version info block read error";
		case unknown_value_type:
			return "Unknown version info block value type";
		case invalid_string_value_length:
			return "Actual string value length is smaller than version info block value length";
		case block_tree_is_too_deep:
			return "Version block tree is too deep";
		default:
			return {};
		}
	}
};

const version_info_reader_error_category version_info_reader_error_category_instance;

using namespace pe_bliss;
using namespace pe_bliss::resources;

bool align_buffer(
	version_info_block_details& block,
	buffers::input_buffer_stateful_wrapper& buf,
	std::uint16_t* const remaining_block_length)
{
	const auto file_offset = buf.get_buffer()->absolute_offset() + buf.rpos();
	auto aligned_file_offset = file_offset;
	if (!utilities::math::align_up_if_safe(aligned_file_offset, sizeof(std::uint32_t)))
	{
		block.add_error(resource_reader_errc::buffer_read_error);
		return false;
	}

	auto difference = static_cast<std::uint16_t>(aligned_file_offset - file_offset);
	if (remaining_block_length)
	{
		if (*remaining_block_length < difference)
		{
			block.add_error(resource_reader_errc::buffer_read_error);
			return false;
		}
		*remaining_block_length -= difference;
	}

	try
	{
		buf.advance_rpos(static_cast<std::int32_t>(difference));
	}
	catch (const std::system_error&)
	{
		block.add_error(resource_reader_errc::buffer_read_error);
		return false;
	}

	return true;
}

bool version_info_from_resource_impl(
	version_info_block_details& block,
	buffers::input_buffer_stateful_wrapper& buf,
	const version_info_read_options& options,
	std::uint32_t depth_left)
{
	{
		const auto file_offset = buf.get_buffer()->absolute_offset() + buf.rpos();
		if (!utilities::math::is_aligned<rva_type>(file_offset))
			block.add_error(version_info_reader_errc::unaligned_version_info_block);
	}

	try
	{
		block.get_descriptor().deserialize(buf, options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		block.add_error(resource_reader_errc::buffer_read_error);
		return false;
	}

	auto remaining_block_length = block.get_descriptor()->length;
	if (remaining_block_length < version_info_block_details::descriptor_type::packed_size)
	{
		block.add_error(version_info_reader_errc::invalid_block_length);
		return false;
	}

	remaining_block_length -= version_info_block_details::descriptor_type::packed_size;
	auto& key = block.get_key().emplace();
	try
	{
		key.deserialize(buf, options.allow_virtual_data, remaining_block_length);
	}
	catch (const std::system_error&)
	{
		block.get_key().reset();
		block.add_error(version_info_reader_errc::key_read_error);
		return false;
	}

	remaining_block_length -= static_cast<std::uint16_t>(key.data_size());

	std::uint32_t value_length = block.get_descriptor()->value_length;
	if (!remaining_block_length)
	{
		//No value, no children
		if (value_length)
		{
			block.add_error(version_info_reader_errc::invalid_value_length);
			return false;
		}

		return true;
	}

	if (value_length)
	{
		if (!align_buffer(block, buf, &remaining_block_length))
			return false;

		if (block.get_value_type() == version_info_value_type::text)
			value_length *= sizeof(char16_t);

		if (value_length > remaining_block_length)
		{
			block.add_error(version_info_reader_errc::invalid_value_length);
			return false;
		}

		try
		{
			switch (block.get_value_type())
			{
			case version_info_value_type::binary:
				{
					auto& value = block.get_value().emplace<buffers::ref_buffer>();
					auto section = buffers::reduce(buf.get_buffer(), buf.rpos(), value_length);
					value.deserialize(section, options.copy_value_memory);
					buf.advance_rpos(value_length);
				}
				break;
			case version_info_value_type::text:
				{
					auto& value = block.get_value().emplace<packed_utf16_c_string>();
					value.deserialize(buf, options.allow_virtual_data, value_length);
					if (value.data_size() != value_length)
						block.add_error(version_info_reader_errc::invalid_string_value_length);
				}
				break;
			default:
				block.add_error(version_info_reader_errc::unknown_value_type);
				return false;
			}
		}
		catch (const std::system_error&)
		{
			block.get_value().emplace<std::monostate>();
			block.add_error(version_info_reader_errc::value_read_error);
			return false;
		}
		
		remaining_block_length -= static_cast<std::uint16_t>(value_length);
	}

	if (!remaining_block_length)
		return true;

	if (!depth_left)
	{
		block.add_error(version_info_reader_errc::block_tree_is_too_deep);

		try
		{
			buf.advance_rpos(remaining_block_length);
		}
		catch (const std::system_error&)
		{
			return false;
		}

		return true;
	}

	try
	{
		auto child_section = buffers::reduce(buf.get_buffer(),
			buf.rpos(), remaining_block_length);
		buffers::input_buffer_stateful_wrapper child_buf(child_section);
		while (child_buf.rpos() != child_buf.size())
		{
			if (!align_buffer(block, child_buf, &remaining_block_length))
				return false;
			auto start_rpos = child_buf.rpos();
			if (!version_info_from_resource_impl(
				block.get_children().emplace_back(), child_buf, options,
				depth_left - 1u))
			{
				return false;
			}
			remaining_block_length -= static_cast<std::uint16_t>(
				child_buf.rpos() - start_rpos);
		}

		buf.advance_rpos(static_cast<std::uint32_t>(child_buf.size()));
	}
	catch (const std::system_error&)
	{
		block.add_error(version_info_reader_errc::child_read_error);
		return false;
	}

	return true;
}

} //namespace

namespace pe_bliss::resources
{

std::error_code make_error_code(version_info_reader_errc e) noexcept
{
	return { static_cast<int>(e), version_info_reader_error_category_instance };
}

version_info_block_details version_info_from_resource(
	buffers::input_buffer_stateful_wrapper& buf,
	const version_info_read_options& options)
{
	version_info_block_details result;
	version_info_from_resource_impl(result, buf, options, options.max_depth);
	if (buf.rpos() != buf.size())
		result.add_error(version_info_reader_errc::excessive_data_in_buffer);
	return result;
}

} //namespace pe_bliss::resources
