#pragma once

namespace buffers
{
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::resources
{

class string_table;

struct [[nodiscard]] string_table_write_options
{
	bool write_virtual_memory = false;
};

void write_string_table(
	const string_table& table,
	buffers::output_buffer_interface& buf,
	const string_table_write_options& options = {});

} //namespace pe_bliss::resources
