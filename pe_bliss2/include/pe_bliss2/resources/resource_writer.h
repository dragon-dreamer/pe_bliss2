#pragma once

#include <string_view>

#include "pe_bliss2/resources/resource_directory.h"
#include "pe_bliss2/resources/resource_types.h"

namespace pe_bliss::resources
{

template<typename Directory>
decltype(auto) try_emplace_resource_data_by_name(Directory& root, resource_type type,
	std::u16string_view name, resource_id_type language)
{
	return root //Type directory
		.try_emplace_entry_by_id(static_cast<resource_id_type>(type),
			directory_entry_contents::directory)
		.get_directory() //Name/ID directory
		.try_emplace_entry_by_name(name,
			directory_entry_contents::directory)
		.get_directory() //Language directory
		.try_emplace_entry_by_id(language,
			directory_entry_contents::data)
		.get_data(); //Data directory
}

template<typename Directory>
decltype(auto) try_emplace_resource_data_by_id(Directory& root, resource_type type,
	resource_id_type id, resource_id_type language)
{
	return root //Type directory
		.try_emplace_entry_by_id(static_cast<resource_id_type>(type),
			directory_entry_contents::directory)
		.get_directory() //Name/ID directory
		.try_emplace_entry_by_id(id,
			directory_entry_contents::directory)
		.get_directory() //Language directory
		.try_emplace_entry_by_id(language,
			directory_entry_contents::data)
		.get_data(); //Data directory
}

} //namespace pe_bliss::resources
