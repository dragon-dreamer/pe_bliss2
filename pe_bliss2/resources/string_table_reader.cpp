#include "pe_bliss2/resources/string_table_reader.h"

#include <cassert>
#include <system_error>

#include "buffers/input_buffer_interface.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/resources/resource_reader_errc.h"
#include "pe_bliss2/resources/string_table.h"

namespace pe_bliss::resources
{

string_table string_table_from_resource(buffers::input_buffer_ptr buf,
	const string_table_read_options& options)
{
	assert(!!buf);

	string_table result;
	buffers::input_buffer_stateful_wrapper_ref ref(*buf);
	result.set_id(options.string_table_id);
	try
	{
		for (auto& str : result.get_list())
			str.deserialize(ref, options.allow_virtual_memory);
	}
	catch (const std::system_error&)
	{
		std::throw_with_nested(pe_error(resource_reader_errc::buffer_read_error));
	}

	return result;
}

} //namespace pe_bliss::resources
