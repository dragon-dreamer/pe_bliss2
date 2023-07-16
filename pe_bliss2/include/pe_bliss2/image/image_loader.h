#pragma once

#include <exception>
#include <system_error>
#include <type_traits>

#include "buffers/input_buffer_interface.h"
#include "pe_bliss2/core/optional_header_validator.h"
#include "pe_bliss2/dos/dos_header_validator.h"
#include "pe_bliss2/image/image.h"
#include "utilities/static_class.h"

namespace pe_bliss
{
class error_list;
} //namespace pe_bliss

namespace pe_bliss::image
{

enum class image_loader_errc
{
	unable_to_load_full_headers_buffer = 1,
	unable_to_load_full_section_buffer
};

struct [[nodiscard]] image_load_options
{
	bool allow_virtual_headers = false;
	bool validate_sections = true;
	bool load_section_data = true;
	bool validate_size_of_image = true;
	bool image_loaded_to_memory = false;
	bool eager_section_data_copy = false;
	bool eager_dos_stub_data_copy = false;
	bool validate_image_base = true;
	bool validate_size_of_optional_header = true;
	bool load_overlay = true;
	bool eager_overlay_data_copy = false;
	bool load_full_headers_buffer = true;
	bool eager_full_headers_buffer_copy = false;
	bool validate_image_signature = true;
	bool load_full_sections_buffer = true;
	bool eager_full_sections_buffer_copy = false;
	dos::dos_header_validation_options dos_header_validation{};
	core::optional_header_validation_options optional_header_validation{};
};

struct [[nodiscard]] image_load_result
{
	pe_bliss::image::image image;
	error_list warnings;
	std::exception_ptr fatal_error;

	[[nodiscard]]
	explicit operator bool() const
	{
		return !fatal_error;
	}
};

class image_loader final : public utilities::static_class
{
public:
	[[nodiscard]]
	static image_load_result load(
		const buffers::input_buffer_ptr& buffer,
		const image_load_options& options = {});

	static void load(
		image& instance,
		error_list& warnings,
		std::exception_ptr& fatal_error,
		const buffers::input_buffer_ptr& buffer,
		const image_load_options& options = {});
};

std::error_code make_error_code(image_loader_errc) noexcept;

} //namespace pe_bliss::image

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::image::image_loader_errc> : true_type {};
} //namespace std
