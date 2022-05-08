#include "pe_bliss2/dos/dos_header.h"

#include <exception>

#include "pe_bliss2/dos/dos_header_errc.h"
#include "pe_bliss2/pe_error.h"

namespace pe_bliss::dos
{

void dos_header::deserialize(buffers::input_buffer_interface& buf,
	bool allow_virtual_memory)
{
	try
	{
		base_struct().deserialize(buf, allow_virtual_memory);
	}
	catch (...)
	{
		std::throw_with_nested(pe_error(
			dos_header_errc::unable_to_read_dos_header));
	}
}

void dos_header::serialize(buffers::output_buffer_interface& buf,
	bool write_virtual_part) const
{
	base_struct().serialize(buf, write_virtual_part);
}

} //namespace pe_bliss::dos
