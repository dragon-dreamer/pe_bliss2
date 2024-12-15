#pragma once

#include "buffers/input_buffer_interface.h"
#include "utilities/static_class.h"

namespace pe_bliss::image
{

enum class detected_format
{
	none,
	pe32,
	pe64
};

class format_detector final : public utilities::static_class
{
public:
	[[nodiscard]]
	static bool looks_like_pe(
		buffers::input_buffer_interface& buffer);

	[[nodiscard]]
	static detected_format detect_format(
		buffers::input_buffer_interface& buffer);
};

} //namespace pe_bliss::image
