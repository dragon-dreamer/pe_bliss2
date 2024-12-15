#include "pe_bliss2/image/format_detector.h"

#include <cstdint>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "pe_bliss2/core/image_signature.h"
#include "pe_bliss2/core/image_signature_validator.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/core/optional_header.h"
#include "pe_bliss2/dos/dos_header.h"
#include "pe_bliss2/dos/dos_header_validator.h"
#include "pe_bliss2/packed_struct.h"

namespace pe_bliss::image
{

namespace
{
bool looks_like_pe_impl(buffers::input_buffer_stateful_wrapper_ref& wrapper)
{
	dos::dos_header dos_hdr;
	dos_hdr.deserialize(wrapper);
	if (dos::validate_magic(dos_hdr).has_error())
		return false;
	if (dos::validate_e_lfanew(dos_hdr).has_error())
		return false;
	wrapper.set_rpos(dos_hdr.get_descriptor()->e_lfanew);

	core::image_signature signature;
	signature.deserialize(wrapper);
	return !core::validate(signature).has_error();
}
} //namespace

bool format_detector::looks_like_pe(
	buffers::input_buffer_interface& buffer)
{
	try
	{
		buffers::input_buffer_stateful_wrapper_ref wrapper(buffer);
		return looks_like_pe_impl(wrapper);
	}
	catch (...)
	{
		return false;
	}
}

detected_format format_detector::detect_format(
	buffers::input_buffer_interface& buffer)
{
	try
	{
		buffers::input_buffer_stateful_wrapper_ref wrapper(buffer);
		if (!looks_like_pe_impl(wrapper))
			return detected_format::none;

		core::file_header fh;
		fh.deserialize(wrapper);

		packed_struct<std::uint16_t> magic_value;
		magic_value.deserialize(wrapper, false);

		if (magic_value.get() == static_cast<uint16_t>(core::optional_header::magic::pe32))
			return detected_format::pe32;
		else if (magic_value.get() == static_cast<uint16_t>(core::optional_header::magic::pe64))
			return detected_format::pe64;
	}
	catch (...)
	{
	}
	return detected_format::none;
}

} //namespace pe_bliss::image
