#include "pe_bliss2/resources/icon_cursor_writer.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>
#include <utility>

#include "buffers/input_buffer_section.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/detail/resources/icon_cursor.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/pe_error.h"

namespace
{

struct icon_cursor_writer_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "icon_writer_reader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::resources::icon_cursor_writer_errc;
		switch (static_cast<pe_bliss::resources::icon_cursor_writer_errc>(ev))
		{
		case different_number_of_headers_and_data:
			return "Different number of icon or cursor group entry headers and corresponding data entries";
		default:
			return {};
		}
	}
};

const icon_cursor_writer_error_category icon_cursor_writer_error_category_instance;
}

namespace pe_bliss::resources
{

std::error_code make_error_code(icon_cursor_writer_errc e) noexcept
{
	return { static_cast<int>(e), icon_cursor_writer_error_category_instance };
}

namespace
{
template<typename FileGroup, typename Group, typename HeaderFormatter>
FileGroup to_file_format_impl(Group&& group,
	const icon_cursor_write_options& options,
	HeaderFormatter&& header_formatter)
{
	const auto& header = group.get_header();
	FileGroup result;
	result.get_header() = header;
	result.get_data_list() = std::forward<Group>(group).get_data_list();
	const auto& res_headers = group.get_resource_group_headers();
	std::size_t current_offset = FileGroup::header_type::packed_size;
	current_offset += FileGroup::resource_group_header_type::packed_size
		* res_headers.size();
	auto& data = result.get_data_list();
	if (data.size() != res_headers.size())
		throw pe_error(icon_cursor_writer_errc::different_number_of_headers_and_data);

	result.get_resource_group_headers().reserve(res_headers.size());
	for (std::size_t i = 0; i != res_headers.size(); ++i)
	{
		const auto& res_header = res_headers[i].get();
		auto& file_header = result.get_resource_group_headers().emplace_back().get();
		header_formatter(res_header, file_header, data[i]);
		auto size = options.write_virtual_part ? data[i].size() : data[i].physical_size();
		file_header.size_in_bytes = static_cast<std::uint32_t>(size);
		file_header.image_offset = static_cast<std::uint32_t>(current_offset);
		current_offset += size;
	}

	return result;
}

template<typename Group>
file_icon_group to_icon_file_format_impl(Group&& group,
	const icon_cursor_write_options& options)
{
	return to_file_format_impl<file_icon_group>(std::forward<Group>(group), options,
	[](const auto& res_header, auto& file_header, auto& /*data*/) {
		file_header.bit_count = res_header.bit_count;
		file_header.color_count = res_header.color_count;
		file_header.height = res_header.height;
		file_header.planes = res_header.planes;
		file_header.reserved = res_header.reserved;
		file_header.size_in_bytes = res_header.size_in_bytes;
		file_header.width = res_header.width;
	});
}

template<typename Group>
file_cursor_group to_cursor_file_format_impl(Group&& group,
	const icon_cursor_write_options& options)
{
	return to_file_format_impl<file_cursor_group>(std::forward<Group>(group), options,
	[&options](const auto& res_header, auto& file_header, auto& data) {
		packed_struct<detail::resources::cursor_hotspots> hotspots;
		{
			buffers::input_buffer_stateful_wrapper_ref ref(*data.data());
			hotspots.deserialize(ref, true);
			auto cursor_data = buffers::reduce(data.data(),
				packed_struct<detail::resources::cursor_hotspots>::packed_size);
			data.deserialize(cursor_data, false);
		}
		file_header.width = static_cast<std::uint8_t>(res_header.width);
		file_header.height = static_cast<std::uint8_t>(res_header.height / 2u);
		file_header.hotspot_x = hotspots->hotspot_x;
		file_header.hotspot_y = hotspots->hotspot_y;
	});
}

template<typename Group>
void write_impl(const Group& group,
	buffers::output_buffer_interface& buf,
	const icon_cursor_write_options& options)
{
	group.get_header().serialize(buf, options.write_virtual_part);
	for (const auto& entry_header : group.get_resource_group_headers())
		entry_header.serialize(buf, options.write_virtual_part);
	for (const auto& data : group.get_data_list())
		data.serialize(buf, options.write_virtual_part);
}
} //namespace

file_icon_group to_file_format(const icon_group& group,
	const icon_cursor_write_options& options)
{
	return to_icon_file_format_impl(group, options);
}

file_icon_group to_file_format(icon_group&& group,
	const icon_cursor_write_options& options)
{
	return to_icon_file_format_impl(std::move(group), options);
}

file_cursor_group to_file_format(const cursor_group& group,
	const icon_cursor_write_options& options)
{
	return to_cursor_file_format_impl(group, options);
}

file_cursor_group to_file_format(cursor_group&& group,
	const icon_cursor_write_options& options)
{
	return to_cursor_file_format_impl(std::move(group), options);
}

void write_icon(const file_icon_group& group,
	buffers::output_buffer_interface& buf,
	const icon_cursor_write_options& options)
{
	write_impl(group, buf, options);
}

void write_cursor(const file_cursor_group& group,
	buffers::output_buffer_interface& buf,
	const icon_cursor_write_options& options)
{
	write_impl(group, buf, options);
}

} //namespace pe_bliss::resources
