#include "pe_bliss2/resources/icon_cursor_reader.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>

#include "buffers/ref_buffer.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/resources/resource_directory.h"
#include "pe_bliss2/resources/resource_reader.h"

namespace
{

struct icon_cursor_reader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "icon_cursor_reader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::resources::icon_cursor_reader_errc;
		switch (static_cast<pe_bliss::resources::icon_cursor_reader_errc>(ev))
		{
		case unable_to_read_header:
			return "Unable to read icon or cursor header";
		case unable_to_read_group_entry_header:
			return "Unable to read icon or cursor group entry header";
		case unable_to_read_data:
			return "Unable to read icon or cursor data";
		default:
			return {};
		}
	}
};

const icon_cursor_reader_error_category icon_cursor_reader_error_category_instance;
}

namespace pe_bliss::resources
{

std::error_code make_error_code(icon_cursor_reader_errc e) noexcept
{
	return { static_cast<int>(e), icon_cursor_reader_error_category_instance };
}

namespace
{
template<typename IconCursor, typename DataLoader>
IconCursor get_icon_cursor(const buffers::ref_buffer& header_buf,
	const icon_cursor_read_options& options,
	const DataLoader& loader)
{
	IconCursor result;
	buffers::input_buffer_stateful_wrapper_ref header_ref(*header_buf.data());
	try
	{
		result.get_header().deserialize(header_ref, options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		std::throw_with_nested(pe_error(
			icon_cursor_reader_errc::unable_to_read_header));
	}

	auto& headers = result.get_resource_group_headers();
	headers.reserve(result.get_header()->count);
	for (std::uint16_t i = 0; i != result.get_header()->count; ++i)
	{
		auto& entry_header = headers.emplace_back();
		try
		{
			entry_header.deserialize(header_ref, options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			std::throw_with_nested(pe_error(
				icon_cursor_reader_errc::unable_to_read_group_entry_header));
		}
	}

	auto& data_list = result.get_data_list();
	data_list.reserve(headers.size());
	for (std::size_t i = 0; i != headers.size(); ++i)
	{
		try
		{
			data_list.emplace_back(loader(headers[i]->number));
		}
		catch (const std::system_error&)
		{
			std::throw_with_nested(pe_error(
				icon_cursor_reader_errc::unable_to_read_data));
		}

		if (!options.allow_virtual_data && data_list.back().virtual_size())
		{
			std::throw_with_nested(pe_error(
				icon_cursor_reader_errc::unable_to_read_data));
		}

		if (options.copy_memory)
			data_list.back().copy_referenced_buffer();
	}

	return result;
}
} //namespace

template<typename... Bases>
icon_group icon_group_from_resource(
	const resource_directory_base<Bases...>& root,
	std::size_t language_index, resource_id_type icon_group_id,
	const icon_cursor_read_options& options)
{
	return get_icon_cursor<icon_group>(get_resource_data_by_id(root, language_index,
		resource_type::icon_group, icon_group_id), options, [language_index, &root](auto number) {
		return get_resource_data_by_id(root, language_index, resource_type::icon, number);
	});
}

template icon_group icon_group_from_resource<>(
	const resource_directory_base<>& root,
	std::size_t language_index, resource_id_type icon_group_id,
	const icon_cursor_read_options& options);
template icon_group icon_group_from_resource<error_list>(
	const resource_directory_base<error_list>& root,
	std::size_t language_index, resource_id_type icon_group_id,
	const icon_cursor_read_options& options);

template<typename... Bases>
icon_group icon_group_from_resource(const resource_directory_base<Bases...>& root,
	std::size_t language_index, std::u16string_view icon_group_name,
	const icon_cursor_read_options& options)
{
	return get_icon_cursor<icon_group>(get_resource_data_by_name(root, language_index,
		resource_type::icon_group, icon_group_name), options, [language_index, &root](auto number) {
		return get_resource_data_by_id(root, language_index, resource_type::icon, number);
	});
}

template icon_group icon_group_from_resource<>(
	const resource_directory_base<>& root,
	std::size_t language_index, std::u16string_view icon_group_name,
	const icon_cursor_read_options& options);
template icon_group icon_group_from_resource<error_list>(
	const resource_directory_base<error_list>& root,
	std::size_t language_index, std::u16string_view icon_group_name,
	const icon_cursor_read_options& options);

template<typename... Bases>
icon_group icon_group_from_resource_by_lang(
	const resource_directory_base<Bases...>& root,
	resource_id_type language, resource_id_type icon_group_id,
	const icon_cursor_read_options& options)
{
	return get_icon_cursor<icon_group>(get_resource_data_by_id(root,
		resource_type::icon_group, icon_group_id, language), options,
		[language, &root](auto number) {
			return get_resource_data_by_id(root, resource_type::icon, number, language);
		});
}

template icon_group icon_group_from_resource_by_lang<>(
	const resource_directory_base<>& root,
	resource_id_type language, resource_id_type icon_group_id,
	const icon_cursor_read_options& options);
template icon_group icon_group_from_resource_by_lang<error_list>(
	const resource_directory_base<error_list>& root,
	resource_id_type language, resource_id_type icon_group_id,
	const icon_cursor_read_options& options);

template<typename... Bases>
icon_group icon_group_from_resource_by_lang(
	const resource_directory_base<Bases...>& root,
	resource_id_type language, std::u16string_view icon_group_name,
	const icon_cursor_read_options& options)
{
	return get_icon_cursor<icon_group>(get_resource_data_by_name(root,
		resource_type::icon_group, icon_group_name, language), options,
		[language, &root](auto number) {
			return get_resource_data_by_id(root, resource_type::icon, number, language);
		});
}

template icon_group icon_group_from_resource_by_lang<>(
	const resource_directory_base<>& root,
	resource_id_type language, std::u16string_view icon_group_name,
	const icon_cursor_read_options& options);
template icon_group icon_group_from_resource_by_lang<error_list>(
	const resource_directory_base<error_list>& root,
	resource_id_type language, std::u16string_view icon_group_name,
	const icon_cursor_read_options& options);

template<typename... Bases>
cursor_group cursor_group_from_resource(
	const resource_directory_base<Bases...>& root,
	std::size_t language_index, resource_id_type cursor_group_id,
	const icon_cursor_read_options& options)
{
	return get_icon_cursor<cursor_group>(get_resource_data_by_id(root, language_index,
		resource_type::cursor_group, cursor_group_id), options, [language_index, &root](auto number) {
		return get_resource_data_by_id(root, language_index, resource_type::cursor, number);
	});
}

template cursor_group cursor_group_from_resource<>(
	const resource_directory_base<>& root,
	std::size_t language_index, resource_id_type cursor_group_id,
	const icon_cursor_read_options& options);
template cursor_group cursor_group_from_resource<error_list>(
	const resource_directory_base<error_list>& root,
	std::size_t language_index, resource_id_type cursor_group_id,
	const icon_cursor_read_options& options);

template<typename... Bases>
cursor_group cursor_group_from_resource(const resource_directory_base<Bases...>& root,
	std::size_t language_index, std::u16string_view cursor_group_name,
	const icon_cursor_read_options& options)
{
	return get_icon_cursor<cursor_group>(get_resource_data_by_name(root, language_index,
		resource_type::cursor_group, cursor_group_name), options, [language_index, &root](auto number) {
		return get_resource_data_by_id(root, language_index, resource_type::cursor, number);
	});
}

template cursor_group cursor_group_from_resource<>(
	const resource_directory_base<>& root,
	std::size_t language_index, std::u16string_view cursor_group_name,
	const icon_cursor_read_options& options);
template cursor_group cursor_group_from_resource<error_list>(
	const resource_directory_base<error_list>& root,
	std::size_t language_index, std::u16string_view cursor_group_name,
	const icon_cursor_read_options& options);

template<typename... Bases>
cursor_group cursor_group_from_resource_by_lang(
	const resource_directory_base<Bases...>& root,
	resource_id_type language, resource_id_type cursor_group_id,
	const icon_cursor_read_options& options)
{
	return get_icon_cursor<cursor_group>(get_resource_data_by_id(root,
		resource_type::cursor_group, cursor_group_id, language), options,
		[language, &root](auto number) {
		return get_resource_data_by_id(root, resource_type::cursor, number, language);
	});
}

template cursor_group cursor_group_from_resource_by_lang<>(
	const resource_directory_base<>& root,
	resource_id_type language, resource_id_type cursor_group_id,
	const icon_cursor_read_options& options);
template cursor_group cursor_group_from_resource_by_lang<error_list>(
	const resource_directory_base<error_list>& root,
	resource_id_type language, resource_id_type cursor_group_id,
	const icon_cursor_read_options& options);

template<typename... Bases>
cursor_group cursor_group_from_resource_by_lang(
	const resource_directory_base<Bases...>& root,
	resource_id_type language, std::u16string_view cursor_group_name,
	const icon_cursor_read_options& options)
{
	return get_icon_cursor<cursor_group>(get_resource_data_by_name(root,
		resource_type::cursor_group, cursor_group_name, language), options,
		[language, &root](auto number) {
		return get_resource_data_by_id(root, resource_type::cursor, number, language);
	});
}

template cursor_group cursor_group_from_resource_by_lang<>(
	const resource_directory_base<>& root,
	resource_id_type language, std::u16string_view cursor_group_name,
	const icon_cursor_read_options& options);
template cursor_group cursor_group_from_resource_by_lang<error_list>(
	const resource_directory_base<error_list>& root,
	resource_id_type language, std::u16string_view cursor_group_name,
	const icon_cursor_read_options& options);

} //namespace pe_bliss::resources
