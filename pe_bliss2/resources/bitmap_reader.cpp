#include "pe_bliss2/resources/bitmap_reader.h"

#include <cstdint>
#include <exception>
#include <string>
#include <system_error>

#include <boost/endian/conversion.hpp>

#include "buffers/input_buffer_interface.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/detail/resources/bitmap.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/pe_error.h"
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
		case invalid_buffer_size:
			return "Invalid buffer size";
		case buffer_read_error:
			return "Buffer read error";
		default:
			return {};
		}
	}
};

const bitmap_reader_error_category bitmap_reader_error_category_instance;
}

namespace pe_bliss::resources
{

std::error_code make_error_code(bitmap_reader_errc e) noexcept
{
	return { static_cast<int>(e), bitmap_reader_error_category_instance };
}

std::vector<std::byte> bitmap_from_resource(buffers::input_buffer_interface& buf,
	bool allow_virtual_memory)
{
	packed_struct<detail::resources::bitmap_info_header> info_header;
	packed_struct<detail::resources::bitmap_file_header> file_header;

	auto buffer_size = allow_virtual_memory ? buf.size() : buf.physical_size();

	if (buffer_size < info_header.packed_size)
		throw pe_error(bitmap_reader_errc::invalid_buffer_size);

	utilities::safe_uint<std::uint32_t> bitmap_size = file_header.packed_size;
	try
	{
		bitmap_size += buffer_size;
	}
	catch (const std::system_error&)
	{
		std::throw_with_nested(pe_error(bitmap_reader_errc::invalid_buffer_size));
	}

	try
	{
		buffers::input_buffer_stateful_wrapper_ref ref(buf);
		info_header.deserialize(ref, allow_virtual_memory);
	}
	catch (const std::system_error&)
	{
		std::throw_with_nested(pe_error(bitmap_reader_errc::buffer_read_error));
	}

	utilities::safe_uint<std::uint32_t> off_bits
		= info_header.packed_size + file_header.packed_size;
	try
	{
		if (((info_header->clr_used << 2u) >> 2u) != info_header->clr_used)
			throw std::system_error(utilities::generic_errc::integer_overflow);

		//If color table is present, skip it
		if (info_header->clr_used)
			off_bits += info_header->clr_used << 2u;
		else if (info_header->bit_count <= 8u)
			off_bits += (1u << info_header->bit_count) << 2u;
	}
	catch (const std::system_error&)
	{
		std::throw_with_nested(pe_error(bitmap_reader_errc::invalid_bitmap_header));
	}

	file_header->type = detail::resources::bm_signature;
	//Offset to bitmap bits
	file_header->off_bits = off_bits.value();
	//Size of bitmap
	file_header->size = bitmap_size.value();

	std::vector<std::byte> result;
	result.resize(bitmap_size.value());
	file_header.serialize(result.data(), result.size(), true);
	try
	{
		buf.read(0u, buffer_size, result.data() + file_header.packed_size);
	}
	catch (const std::system_error&)
	{
		std::throw_with_nested(pe_error(bitmap_reader_errc::buffer_read_error));
	}
	return result;
}

} //namespace pe_bliss::resources
