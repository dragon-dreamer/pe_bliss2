#include "pe_bliss2/core/overlay.h"

#include <algorithm>
#include <exception>
#include <memory>
#include <string>
#include <system_error>

#include "buffers/input_buffer_section.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/safe_uint.h"

namespace
{

struct overlay_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "overlay";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::core::overlay_errc;
		switch (static_cast<pe_bliss::core::overlay_errc>(ev))
		{
		case unable_to_read_overlay:
			return "Unable to read overlay";
		default:
			return {};
		}
	}
};

const overlay_error_category overlay_error_category_instance;

} //namespace

namespace pe_bliss::core
{

std::error_code make_error_code(overlay_errc e) noexcept
{
	return { static_cast<int>(e), overlay_error_category_instance };
}

void overlay::deserialize(std::uint64_t section_raw_data_last_offset,
	std::uint32_t size_of_headers,
	std::size_t initial_buffer_pos,
	const buffers::input_buffer_ptr& buffer,
	bool eager_copy)
{
	utilities::safe_uint<std::uint64_t> last_image_offset = section_raw_data_last_offset;
	last_image_offset = (std::max<std::uint64_t>)(last_image_offset.value(), size_of_headers);

	try
	{
		last_image_offset += initial_buffer_pos;
		auto buffer_size = buffer->size();
		if (last_image_offset >= buffer_size)
			return;

		std::size_t overlay_size = buffer_size - last_image_offset.value();
		ref_buffer::deserialize(
			buffers::reduce(buffer, last_image_offset.value(), overlay_size),
			eager_copy);
	}
	catch (...)
	{
		std::throw_with_nested(pe_error(overlay_errc::unable_to_read_overlay));
	}
}

} //namespace pe_bliss::core
