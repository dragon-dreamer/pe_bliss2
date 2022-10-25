#include "pe_bliss2/core/image_signature.h"

#include <exception>
#include <system_error>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "pe_bliss2/core/image_signature_errc.h"
#include "pe_bliss2/pe_error.h"

namespace pe_bliss::core
{

void image_signature::deserialize(buffers::input_buffer_stateful_wrapper_ref& buf,
	bool allow_virtual_memory)
{
	try
	{
		get_descriptor().deserialize(buf, allow_virtual_memory);
	}
	catch (const std::system_error&)
	{
		std::throw_with_nested(pe_error(
			image_signature_errc::unable_to_read_pe_signature));
	}
}

void image_signature::serialize(buffers::output_buffer_interface& buf,
	bool write_virtual_part) const
{
	get_descriptor().serialize(buf, write_virtual_part);
}

} //namespace pe_bliss::core
