#include "pe_bliss2/resources/string_table.h"

#include <cassert>

namespace pe_bliss::resources
{

string_table::id_type string_table::string_to_table_id(
	id_type string_id) noexcept
{
	return (string_id >> 4u) + 1u;
}

string_table::id_type string_table::table_to_string_id(
	id_type table_id, std::uint8_t index) noexcept
{
	assert(table_id > 0);
	assert(index < max_string_count);
	return ((table_id - 1u) << 4u) + index;
}

} //namespace pe_bliss::resources
