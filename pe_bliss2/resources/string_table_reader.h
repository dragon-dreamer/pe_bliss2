#pragma once

#include "pe_bliss2/resources/string_table.h"

namespace buffers
{
class input_buffer_stateful_wrapper_ref;
} //namespace buffers

namespace pe_bliss::resources
{

struct [[nodiscard]] string_table_read_options
{
	string_table::id_type string_table_id{};
	bool allow_virtual_memory = false;
};

[[nodiscard]]
string_table string_table_from_resource(
	buffers::input_buffer_stateful_wrapper_ref& buf,
	const string_table_read_options& options = {});

} //namespace pe_bliss::resources
