#pragma once

#include "buffers/input_buffer_interface.h"

#include "pe_bliss2/resources/string_table.h"

namespace pe_bliss::resources
{

struct [[nodiscard]] string_table_read_options
{
	string_table::id_type string_table_id{};
	bool allow_virtual_memory = false;
};

[[nodiscard]]
string_table string_table_from_resource(buffers::input_buffer_ptr buf,
	const string_table_read_options& options = {});

} //namespace pe_bliss::resources
