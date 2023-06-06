#include "pe_bliss2/dos/dos_header.h"

#include <exception>
#include <system_error>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "pe_bliss2/dos/dos_header_errc.h"
#include "pe_bliss2/pe_error.h"

namespace pe_bliss::dos
{

void dos_header::deserialize(buffers::input_buffer_stateful_wrapper_ref& buf,
	bool allow_virtual_data)
{
	try
	{
		get_descriptor().deserialize(buf, allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		std::throw_with_nested(pe_error(
			dos_header_errc::unable_to_read_dos_header));
	}
}

void dos_header::serialize(buffers::output_buffer_interface& buf,
	bool write_virtual_part) const
{
	get_descriptor().serialize(buf, write_virtual_part);
}

} //namespace pe_bliss::dos
