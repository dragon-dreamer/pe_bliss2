#include "pe_bliss2/core/optional_header_validator.h"

#include <bit>

#include "pe_bliss2/core/optional_header.h"
#include "pe_bliss2/core/optional_header_errc.h"
#include "pe_bliss2/detail/image_data_directory.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/section/section_header.h"

#include "utilities/math.h"

namespace pe_bliss::core
{

pe_error_wrapper validate_address_of_entry_point(
	const optional_header& header, bool is_dll) noexcept
{
	auto entry_point = header.get_raw_address_of_entry_point();
	if (!entry_point && !is_dll)
		return optional_header_errc::invalid_address_of_entry_point;
	// Under Windows 8, AddressOfEntryPoint is not allowed to be smaller
	// than SizeOfHeaders, except if it is 0.
	if (entry_point && entry_point < header.get_raw_size_of_headers())
		return optional_header_errc::invalid_address_of_entry_point;
	return {};
}

pe_error_wrapper validate_image_base(
	const optional_header& header, bool has_relocations) noexcept
{
	static constexpr std::uint32_t image_base_multiple = 0x10000u;
	static constexpr std::uint64_t max_nonreloc_image_base = 0x80000000u;

	//Can be zero under WinXP
	if (header.get_raw_image_base() % image_base_multiple)
		return optional_header_errc::unaligned_image_base;
	if (!has_relocations)
	{
		std::uint64_t image_base_sum = header.get_raw_image_base()
			+ header.get_raw_size_of_image();
		if (image_base_sum >= max_nonreloc_image_base)
			return optional_header_errc::too_large_image_base;
	}
	return {};
}

pe_error_wrapper validate_file_alignment(const optional_header& header) noexcept
{
	auto section_alignment = header.get_raw_section_alignment();
	auto file_alignment = header.get_raw_file_alignment();

	if (!std::has_single_bit(file_alignment))
		return optional_header_errc::incorrect_file_alignment;

	if (!header.is_low_alignment())
	{
		if (file_alignment < optional_header::minimum_file_alignment
			|| file_alignment > section_alignment)
		{
			return optional_header_errc::file_alignment_out_of_range;
		}
	}
	return {};
}

pe_error_wrapper validate_section_alignment(const optional_header& header) noexcept
{
	auto section_alignment = header.get_raw_section_alignment();
	auto file_alignment = header.get_raw_file_alignment();

	if (!std::has_single_bit(section_alignment))
		return optional_header_errc::incorrect_section_alignment;

	if (!header.is_low_alignment())
	{
		if (section_alignment < optional_header::minimum_section_alignment
			|| file_alignment > section_alignment)
		{
			return optional_header_errc::section_alignment_out_of_range;
		}
	}
	return {};
}

pe_error_wrapper validate_subsystem_version(const optional_header& header) noexcept
{
	//TODO: if SubsystemVersion is 6.30, the loader enforces the presence of the LoadConfig entry,
	//with a valid cookie, unless GuardFlags are set to IMAGE_GUARD_SECURITY_COOKIE_UNUSED.
	auto major_subsystem_version = header.get_raw_major_subsystem_version();
	auto minor_subsystem_version = header.get_raw_minor_subsystem_version();

	if (major_subsystem_version < optional_header::min_major_subsystem_version
		|| (major_subsystem_version == optional_header::min_major_subsystem_version
			&& minor_subsystem_version < optional_header::min_minor_subsystem_version))
	{
		return optional_header_errc::too_low_subsystem_version;
	}

	return {};
}

pe_error_wrapper validate_size_of_heap(const optional_header& header) noexcept
{
	if (header.get_raw_size_of_heap_commit() > header.get_raw_size_of_heap_reserve())
		return optional_header_errc::invalid_size_of_heap;
	return {};
}

pe_error_wrapper validate_size_of_stack(const optional_header& header) noexcept
{
	if (header.get_raw_size_of_stack_commit() > header.get_raw_size_of_stack_reserve())
		return optional_header_errc::invalid_size_of_stack;
	return {};
}

pe_error_wrapper validate_size_of_headers(const optional_header& header) noexcept
{
	if (!header.is_low_alignment() && header.get_raw_section_alignment()
		< header.get_raw_size_of_headers())
	{
		return optional_header_errc::invalid_size_of_headers;
	}
	if (header.get_raw_size_of_image() < header.get_raw_size_of_headers())
		return optional_header_errc::invalid_size_of_headers;
	return {};
}

bool validate(const optional_header& header,
	const optional_header_validation_options& options,
	bool is_dll, error_list& errors) noexcept
{
	pe_error_wrapper result;
	if (options.validate_address_of_entry_point
		&& (result = validate_address_of_entry_point(header, is_dll)))
	{
		errors.add_error(result);
	}

	if (options.validate_alignments)
	{
		if ((result = validate_file_alignment(header)))
			errors.add_error(result);

		if ((result = validate_section_alignment(header)))
			errors.add_error(result);
	}

	if (options.validate_subsystem_version
		&& (result = validate_subsystem_version(header)))
	{
		errors.add_error(result);
	}

	if (options.validate_size_of_heap
		&& (result = validate_size_of_heap(header)))
	{
		errors.add_error(result);
	}

	if (options.validate_size_of_stack
		&& (result = validate_size_of_stack(header)))
	{
		errors.add_error(result);
	}

	if (options.validate_size_of_headers
		&& (result = validate_size_of_headers(header)))
	{
		errors.add_error(result);
	}

	return !errors.has_errors();
}

pe_error_wrapper validate_size_of_optional_header(
	std::uint16_t size_of_optional_header,
	const optional_header& oh) noexcept
{
	constexpr auto data_dir_size = detail::packed_reflection
		::get_type_size<detail::image_data_directory>();
	if (size_of_optional_header < oh.get_size_of_structure()
		+ data_dir_size * oh.get_number_of_rva_and_sizes())
	{
		return optional_header_errc::invalid_size_of_optional_header;
	}

	return {};
}

pe_error_wrapper validate_size_of_image(
	const section::section_header* last_section,
	const optional_header& oh) noexcept
{
	if (!last_section) [[unlikely]]
		return {};

	auto real_size_of_image = last_section->base_struct()->virtual_address;
	if (!utilities::math::add_if_safe(real_size_of_image,
		last_section->get_virtual_size(oh.get_raw_section_alignment())))
	{
		return optional_header_errc::invalid_size_of_image;
	}

	if (oh.get_raw_size_of_image() != real_size_of_image)
		return optional_header_errc::invalid_size_of_image;

	return {};
}

} //namespace pe_bliss::core
