#include "pe_bliss2/image/checksum.h"

#include <cstdint>
#include <string>
#include <system_error>

#include "buffers/input_buffer_interface.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_buffer_section.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/core/image_signature.h"
#include "pe_bliss2/core/optional_header.h"
#include "pe_bliss2/detail/image_optional_header.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/detail/packed_serialization.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/math.h"
#include "utilities/safe_uint.h"

namespace
{

struct checksum_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "checksum";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::image::checksum_errc;
		switch (static_cast<pe_bliss::image::checksum_errc>(ev))
		{
		case invalid_checksum_offset:
			return "Invalid checksum field offset";
		case unaligned_checksum:
			return "Checksum field is not aligned on DWORD boundary";
		case unaligned_buffer:
			return "Headers, section or overlay image buffer size is not DWORD-aligned";
		default:
			return {};
		}
	}
};

const checksum_error_category checksum_error_category_instance;

std::uint64_t calculate_checksum_impl(std::uint64_t checksum,
	buffers::input_buffer_interface& buf)
{
	static constexpr std::uint64_t max_value = 0x100000000ull;

	auto physical_size = buf.physical_size();
	if (!utilities::math::is_aligned<sizeof(std::uint32_t)>(physical_size))
		throw pe_bliss::pe_error(pe_bliss::image::checksum_errc::unaligned_buffer);

	buffers::input_buffer_stateful_wrapper_ref ref(buf);
	static constexpr std::size_t temp_buffer_size
		= sizeof(pe_bliss::image::image_checksum_type) * 128u;
	std::array<std::byte, temp_buffer_size> temp;
	pe_bliss::image::image_checksum_type dword{};
	while (physical_size)
	{
		auto read_bytes = std::min(physical_size, temp_buffer_size);
		physical_size -= read_bytes;
		ref.read(read_bytes, temp.data());
		const auto* ptr = temp.data();
		for (std::size_t i = 0; i != read_bytes;
			i += sizeof(pe_bliss::image::image_checksum_type),
			ptr += sizeof(pe_bliss::image::image_checksum_type))
		{
			pe_bliss::detail::packed_serialization<>::deserialize(dword, ptr);
			checksum = (checksum & 0xffffffffull) + dword + (checksum >> 32ull);
			if (checksum > max_value)
				checksum = (checksum & 0xffffffffull) + (checksum >> 32ull);
		}
	}

	return checksum;
}

} //namespace

namespace pe_bliss::image
{

std::error_code make_error_code(checksum_errc e) noexcept
{
	return { static_cast<int>(e), checksum_error_category_instance };
}

std::uint32_t get_checksum_offset(const image& instance)
{
	static constexpr auto optional_header_checksum_offset = detail::packed_reflection
		::get_field_offset<&detail::image_optional_header_32::checksum>();
	static_assert(optional_header_checksum_offset == detail::packed_reflection
		::get_field_offset<&detail::image_optional_header_64::checksum>());

	utilities::safe_uint checksum_offset
		= instance.get_dos_header().get_descriptor()->e_lfanew;
	try
	{
		checksum_offset += core::file_header::descriptor_type::packed_size;
		checksum_offset += core::image_signature::descriptor_type::packed_size;
		checksum_offset += sizeof(core::optional_header::magic_type);
		checksum_offset += optional_header_checksum_offset;
	}
	catch (const std::system_error&)
	{
		throw pe_error(checksum_errc::invalid_checksum_offset);
	}

	if (!utilities::math::is_aligned<sizeof(std::uint32_t)>(
		checksum_offset.value()))
	{
		throw pe_error(checksum_errc::unaligned_checksum);
	}

	return checksum_offset.value();
}

image_checksum_type calculate_checksum(const image& instance)
{
	const auto checksum_offset = get_checksum_offset(instance);

	std::uint64_t file_size = instance.get_full_headers_buffer().physical_size();

	buffers::input_buffer_ptr before_checksum, after_checksum;
	try
	{
		auto headers_buffer = instance.get_full_headers_buffer().data();
		before_checksum = buffers::reduce(headers_buffer, 0u, checksum_offset);
		after_checksum = buffers::reduce(headers_buffer,
			checksum_offset + sizeof(image_checksum_type));
	}
	catch (const std::system_error&)
	{
		throw pe_error(checksum_errc::invalid_checksum_offset);
	}

	auto checksum = calculate_checksum_impl(0ull, *before_checksum);
	checksum = calculate_checksum_impl(checksum, *after_checksum);

	auto full_section_buf = instance.get_full_sections_buffer().data();
	if (full_section_buf->size())
	{
		checksum = calculate_checksum_impl(checksum, *full_section_buf);
		file_size += full_section_buf->physical_size();
	}
	else
	{
		for (const auto& section : instance.get_section_data_list())
		{
			checksum = calculate_checksum_impl(checksum, *section.data());
			file_size += section.physical_size();
		}
	}

	checksum = calculate_checksum_impl(checksum, *instance.get_overlay().data());
	file_size += instance.get_overlay().physical_size();

	//Finalize checksum
	checksum = (checksum & 0xffffull) + (checksum >> 16ull);
	checksum = checksum + (checksum >> 16ull);
	checksum = checksum & 0xffffull;
	checksum += file_size;
	return static_cast<image_checksum_type>(checksum);
}

} //namespace pe_bliss::image
