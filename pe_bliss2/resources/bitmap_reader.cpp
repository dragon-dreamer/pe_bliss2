#include "pe_bliss2/resources/bitmap_reader.h"

#include <cassert>
#include <cstdint>
#include <exception>
#include <string>
#include <system_error>

#include "buffers/input_buffer_interface.h"
#include "buffers/input_buffer_section.h"
#include "pe_bliss2/detail/resources/bitmap.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/resources/bitmap.h"
#include "pe_bliss2/resources/resource_reader_errc.h"
#include "utilities/generic_error.h"
#include "utilities/safe_uint.h"

namespace
{
struct bitmap_reader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "bitmap_reader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::resources::bitmap_reader_errc;
		switch (static_cast<pe_bliss::resources::bitmap_reader_errc>(ev))
		{
		case invalid_bitmap_header:
			return "Invalid bitmap header";
		default:
			return {};
		}
	}
};

const bitmap_reader_error_category bitmap_reader_error_category_instance;
} // namespace

namespace pe_bliss::resources
{

std::error_code make_error_code(bitmap_reader_errc e) noexcept
{
	return { static_cast<int>(e), bitmap_reader_error_category_instance };
}

bitmap bitmap_from_resource(const buffers::input_buffer_ptr& buf,
	const bitmap_read_options& options)
{
	assert(!!buf);

	auto buffer_size = options.allow_virtual_memory
		? buf->size() : buf->physical_size();

	if (buffer_size < bitmap::info_header_type::packed_size)
		throw pe_error(resource_reader_errc::invalid_buffer_size);

	utilities::safe_uint<std::uint32_t> bitmap_size
		= bitmap::file_header_type::packed_size;
	try
	{
		bitmap_size += buffer_size;
	}
	catch (const std::system_error&)
	{
		std::throw_with_nested(pe_error(resource_reader_errc::invalid_buffer_size));
	}

	bitmap result;
	try
	{
		buffers::input_buffer_stateful_wrapper_ref ref(*buf);
		result.get_info_header().deserialize(ref, options.allow_virtual_memory);
	}
	catch (const std::system_error&)
	{
		std::throw_with_nested(pe_error(resource_reader_errc::buffer_read_error));
	}

	utilities::safe_uint<std::uint32_t> off_bits
		= bitmap::info_header_type::packed_size
		+ bitmap::file_header_type::packed_size;
	try
	{
		if (((result.get_info_header()->clr_used << 2u) >> 2u)
			!= result.get_info_header()->clr_used)
		{
			throw std::system_error(utilities::generic_errc::integer_overflow);
		}

		//If color table is present, skip it
		if (result.get_info_header()->clr_used)
			off_bits += result.get_info_header()->clr_used << 2u;
		else if (result.get_info_header()->bit_count <= 8u)
			off_bits += (1u << result.get_info_header()->bit_count) << 2u;
	}
	catch (const std::system_error&)
	{
		std::throw_with_nested(pe_error(bitmap_reader_errc::invalid_bitmap_header));
	}

	result.get_file_header()->type = detail::resources::bm_signature;
	//Offset to bitmap bits
	result.get_file_header()->off_bits = off_bits.value();
	//Size of bitmap
	result.get_file_header()->size = bitmap_size.value();

	result.get_buffer().deserialize(buffers::reduce(buf,
		bitmap::info_header_type::packed_size), options.copy_memory);
	return result;
}

} //namespace pe_bliss::resources
