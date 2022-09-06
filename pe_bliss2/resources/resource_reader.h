#pragma once

#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string_view>
#include <variant>

#include "pe_bliss2/resources/resource_directory.h"
#include "pe_bliss2/resources/resource_types.h"

namespace pe_bliss::resources
{

template<typename... Bases>
[[nodiscard]]
resource_type_list_type list_resource_types(const resource_directory_base<Bases...>& dir);

template<typename Directory>
[[nodiscard]]
decltype(auto) get_directory_by_type(Directory& root, resource_type type)
{
	return root.entry_by_id(static_cast<resource_id_type>(type)).get_directory();
}

template<typename Directory>
[[nodiscard]]
decltype(auto) get_directory_by_name(Directory& root, std::u16string_view name)
{
	return root.entry_by_name(name).get_directory();
}

template<typename Directory>
[[nodiscard]]
decltype(auto) get_resource_data_by_name(Directory& root, resource_type type,
	std::u16string_view name, resource_id_type language)
{
	return root //Type directory
		.entry_by_id(static_cast<resource_id_type>(type))
		.get_directory() //Name/ID directory
		.entry_by_name(name)
		.get_directory() //Language directory
		.entry_by_id(language)
		.get_data() //Data directory
		.get_raw_data();
}

template<typename Directory>
[[nodiscard]]
decltype(auto) get_resource_data_by_id(Directory& root, resource_type type,
	resource_id_type id, resource_id_type language)
{
	return root //Type directory
		.entry_by_id(static_cast<resource_id_type>(type))
		.get_directory() //Name/ID directory
		.entry_by_id(id)
		.get_directory() //Language directory
		.entry_by_id(language)
		.get_data() //Data directory
		.get_raw_data();
}

template<typename Directory>
[[nodiscard]]
decltype(auto) get_resource_data_by_name(Directory& root, std::size_t language_index,
	resource_type type, std::u16string_view name)
{
	try
	{
		return root //Type directory
			.entry_by_id(static_cast<resource_id_type>(type))
			.get_directory() //Name/ID directory
			.entry_by_name(name)
			.get_directory() //Language directory
			.get_entries().at(language_index)
			.get_data() //Data directory
			.get_raw_data();
	}
	catch (const std::out_of_range&)
	{
		std::throw_with_nested(
			pe_error(resource_directory_errc::entry_does_not_exist));
	}
}

template<typename Directory>
[[nodiscard]]
decltype(auto) get_resource_data_by_id(Directory& root, std::size_t language_index,
	resource_type type, resource_id_type id)
{
	try
	{
		return root //Type directory
			.entry_by_id(static_cast<resource_id_type>(type))
			.get_directory() //Name/ID directory
			.entry_by_id(id)
			.get_directory() //Language directory
			.get_entries().at(language_index)
			.get_data() //Data directory
			.get_raw_data();
	}
	catch (const std::out_of_range&)
	{
		std::throw_with_nested(
			pe_error(resource_directory_errc::entry_does_not_exist));
	}
}

template<typename Directory, typename Func>
bool for_each_resource(Directory& root, resource_type type, Func&& func)
{
	auto* entry = root.try_entry_by_id(static_cast<resource_id_type>(type));
	if (!entry || !entry->has_directory())
		return false;

	for (auto& name_id_entry : entry->get_directory().get_entries())
	{
		if (!name_id_entry.has_directory()
			|| !(name_id_entry.is_named() || name_id_entry.has_id()))
		{
			continue;
		}

		for (auto& language_entry
			: name_id_entry.get_directory().get_entries())
		{
			if (!language_entry.has_data() || !language_entry.has_id())
				continue;

			if (std::forward<Func>(func)(name_id_entry.get_name_or_id(),
				language_entry.get_id(),
				language_entry.get_data().get_raw_data()))
			{
				return true;
			}
		}
	}

	return false;
}

} //namespace pe_bliss::resources
