#include "pe_bliss2/resources/resource_reader.h"

#include <variant>
#include <unordered_set>

#include "pe_bliss2/error_list.h"

namespace pe_bliss::resources
{

template<typename... Bases>
resource_type_list_type list_resource_types(const resource_directory_base<Bases...>& dir)
{
	resource_type_list_type result;
	std::unordered_set<resource_id_type> visited;
	for (const auto& entry : dir.get_entries())
	{
		const auto* id = std::get_if<resource_id_type>(&entry.get_name_or_id());
		if (id && visited.emplace(*id).second)
			result.emplace_back(static_cast<resource_type>(*id));
	}
	return result;
}

template resource_type_list_type list_resource_types<>(
	const resource_directory_base<>& dir);
template resource_type_list_type list_resource_types<error_list>(
	const resource_directory_base<error_list>& dir);

} //namespace pe_bliss::resources
