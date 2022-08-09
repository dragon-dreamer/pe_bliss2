#include "pe_bliss2/rich/rich_header.h"

#include <climits>
#include <cstddef>
#include <exception>
#include <limits>
#include <string>
#include <system_error>

#include "buffers/input_buffer_stateful_wrapper.h"

#include "pe_bliss2/detail/image_dos_header.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/detail/rich/rich_header_utils.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/rich/rich_compid.h"

#include "utilities/math.h"

namespace
{

struct rich_header_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "rich_header";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::rich::rich_header_errc;
		switch (static_cast<pe_bliss::rich::rich_header_errc>(ev))
		{
		case invalid_rich_header_offset:
			return "Rich header absolute offset is greater than image buffer size";
		case unaligned_rich_header_offset:
			return "Unaligned Rich header offset (must be DWORD-aligned)";
		case unable_to_calculate_rich_signature:
			return "Unable to calculate Rich signature";
		default:
			return {};
		}
	}
};

const rich_header_error_category rich_header_error_category_instance;

} //namespace

namespace pe_bliss::rich
{

std::error_code make_error_code(rich_header_errc e) noexcept
{
	return { static_cast<int>(e), rich_header_error_category_instance };
}

rich_header::checksum_type rich_header::calculate_checksum(
	buffers::input_buffer_stateful_wrapper_ref& image_buffer) const
{
	static constexpr auto e_lfanew_offset = detail::packed_reflection
		::get_field_offset<&detail::image_dos_header::e_lfanew>();
	static constexpr auto dos_header_data_size
		= detail::packed_reflection::get_type_size<detail::image_dos_header>();

	auto checksum = static_cast<checksum_type>(dos_header_data_size);
	if (dans_relative_offset_ > (std::numeric_limits<checksum_type>::max)()
		|| !utilities::math::add_if_safe(checksum,
			static_cast<checksum_type>(dans_relative_offset_)))
	{
		throw pe_error(rich_header_errc::invalid_rich_header_offset);
	}

	if (checksum % sizeof(checksum_type))
		throw pe_error(rich_header_errc::unaligned_rich_header_offset);

	auto total_offset = dos_header_data_size + dans_relative_offset_;
	if (image_buffer.size() < total_offset)
		throw pe_error(rich_header_errc::invalid_rich_header_offset);

	image_buffer.set_rpos(0u);
	std::byte value{};
	for (std::size_t i = 0; i != total_offset; ++i) {
		if (image_buffer.read(sizeof(value), &value) != sizeof(value)) [[unlikely]]
			throw pe_error(rich_header_errc::unable_to_calculate_rich_signature);

		if (i >= e_lfanew_offset && i < e_lfanew_offset
			+ sizeof(detail::image_dos_header::e_lfanew))
			[[unlikely]]
		{
			continue;
		}

		checksum += std::rotl(std::to_integer<std::uint32_t>(
			value), static_cast<std::uint8_t>(i));
	}

	for (const auto& compid : compids_)
		checksum += rich::get_checksum(compid);

	return checksum;
}

} //namespace pe_bliss::rich
