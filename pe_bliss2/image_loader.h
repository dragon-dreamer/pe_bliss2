#pragma once

#include "buffers/input_buffer_interface.h"
#include "pe_bliss2/dos/dos_header_validator.h"
#include "pe_bliss2/image.h"
#include "pe_bliss2/optional_header.h"
#include "utilities/static_class.h"

namespace pe_bliss
{

struct image_load_options
{
	bool allow_virtual_headers = false;
	bool validate_sections = true;
	bool load_section_data = true;
	bool validate_size_of_image = true;
	bool image_loaded_to_memory = false;
	bool eager_section_data_copy = false;
	bool eager_dos_stub_data_copy = false;
	bool validate_image_base = true;
	bool load_overlay = true;
	bool eager_overlay_data_copy = false;
	bool load_full_headers_buffer = true;
	bool eager_full_headers_buffer_copy = false;
	dos::dos_header_validation_options dos_header_validation{};
	optional_header_validation_options optional_header_validation{};
};

class image_loader : public utilities::static_class
{
public:
	[[nodiscard]]
	static image load(const buffers::input_buffer_ptr& buffer,
		const image_load_options& options);
};

} //namespace pe_bliss
