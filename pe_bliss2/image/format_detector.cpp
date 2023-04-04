#include "pe_bliss2/image/format_detector.h"

#include "buffers/input_buffer_stateful_wrapper.h"
#include "pe_bliss2/core/image_signature.h"
#include "pe_bliss2/core/image_signature_validator.h"
#include "pe_bliss2/dos/dos_header.h"
#include "pe_bliss2/dos/dos_header_validator.h"

namespace pe_bliss::image
{

bool format_detector::looks_like_pe(
	buffers::input_buffer_interface& buffer)
{
	try
	{
		buffers::input_buffer_stateful_wrapper_ref wrapper(buffer);

		dos::dos_header dos_hdr;
		dos_hdr.deserialize(wrapper);
		dos::validate_magic(dos_hdr).throw_on_error();
		dos::validate_e_lfanew(dos_hdr).throw_on_error();
		wrapper.set_rpos(dos_hdr.get_descriptor()->e_lfanew);

		core::image_signature signature;
		signature.deserialize(wrapper);
		core::validate(signature).throw_on_error();
		return true;
	}
	catch (...)
	{
		return false;
	}
}

} //namespace pe_bliss::image
