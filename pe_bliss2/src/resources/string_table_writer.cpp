#include "pe_bliss2/resources/string_table_writer.h"

#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/resources/string_table.h"

namespace pe_bliss::resources
{

void write_string_table(
	const string_table& table,
	buffers::output_buffer_interface& buf,
	const string_table_write_options& options)
{
	for (const auto& str : table.get_list())
		str.serialize(buf, options.write_virtual_memory);
}

} //namespace pe_bliss::resources
