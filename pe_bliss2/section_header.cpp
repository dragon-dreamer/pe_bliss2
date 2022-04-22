#include "pe_bliss2/section_header.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/section_table.h"
#include "utilities/math.h"

namespace pe_bliss
{

void section_header::deserialize(buffers::input_buffer_interface& buf,
	bool allow_virtual_memory)
{
	try
	{
		base_struct().deserialize(buf, allow_virtual_memory);
	}
	catch (...)
	{
		std::throw_with_nested(pe_error(
			section_table_errc::unable_to_read_section_table));
	}
}

void section_header::serialize(buffers::output_buffer_interface& buf,
	bool write_virtual_part) const
{
	base_struct().serialize(buf, write_virtual_part);
}

rva_type section_header::rva_from_section_offset(
	std::uint32_t raw_offset_from_section_start, std::uint32_t section_alignment) const
{
	if (raw_offset_from_section_start >= get_virtual_size(section_alignment))
		throw pe_error(section_table_errc::invalid_section_offset);

	return get_rva() + raw_offset_from_section_start;
}

std::string_view section_header::get_name() const noexcept
{
	auto begin = base_struct()->name;
	auto end = begin + sizeof(detail::image_section_header::name) - 1;
	while (end >= begin && !*end)
		--end;

	return { reinterpret_cast<const char*>(base_struct()->name),
		static_cast<std::size_t>(end - begin + 1) };
}

section_header& section_header::set_name(std::string_view name)
{
	if (name.size() > sizeof(detail::image_section_header::name))
		throw pe_error(utilities::generic_errc::buffer_overrun);

	std::fill(std::begin(base_struct()->name),
		std::end(base_struct()->name), static_cast<std::uint8_t>(0u));
	std::copy(std::begin(name), std::end(name), base_struct()->name);
	return *this;
}

rva_type section_header::get_rva() const noexcept
{
	return base_struct()->virtual_address;
}

section_header& section_header::set_rva(std::uint32_t rva) noexcept
{
	base_struct()->virtual_address = rva;
	return *this;
}

bool section_header::check_raw_address() const noexcept
{
	if (base_struct()->size_of_raw_data == 0 && base_struct()->virtual_size == 0)
		return false;
	return base_struct()->size_of_raw_data == 0
		|| base_struct()->pointer_to_raw_data != 0;
}

std::uint32_t section_header::get_raw_size(
	std::uint32_t section_alignment) const noexcept
{
	auto raw_size = base_struct()->size_of_raw_data;
	auto virtual_size = get_virtual_size(section_alignment);
	if (raw_size > virtual_size)
		raw_size = virtual_size;
	return raw_size;
}

bool section_header::check_raw_size(
	std::uint32_t section_alignment) const noexcept
{
	return get_raw_size(section_alignment) < two_gb_size;
}

bool section_header::check_virtual_size() const noexcept
{
	auto size = base_struct()->virtual_size;
	return size < two_gb_size && size;
}

std::uint32_t section_header::get_virtual_size(
	std::uint32_t section_alignment) const noexcept
{
	auto virtual_size = base_struct()->virtual_size;
	if (!virtual_size)
		virtual_size = base_struct()->size_of_raw_data;
	if (!std::has_single_bit(section_alignment))
		return virtual_size;

	(void)utilities::math::align_up_if_safe(
		virtual_size, section_alignment);

	return virtual_size;
}

bool section_header::check_raw_size_alignment(std::uint32_t section_alignment,
	std::uint32_t file_alignment,
	bool last_section) const noexcept
{
	if (!std::has_single_bit(section_alignment)
		|| !std::has_single_bit(file_alignment))
	{
		return false;
	}
	return last_section || (get_raw_size(section_alignment) % file_alignment) == 0;
}

bool section_header::check_raw_size_bounds(
	std::uint32_t section_alignment) const noexcept
{
	return utilities::math::is_sum_safe(get_raw_size(section_alignment),
		get_pointer_to_raw_data());
}

bool section_header::check_virtual_size_bounds(
	std::uint32_t section_alignment) const noexcept
{
	return utilities::math::is_sum_safe(
		get_virtual_size(section_alignment), get_rva());
}

bool section_header::check_raw_address_alignment(
	std::uint32_t file_alignment) const noexcept
{
	if (!std::has_single_bit(file_alignment))
		return false;
	return (get_pointer_to_raw_data() % file_alignment) == 0;
}

bool section_header::check_virtual_address_alignment(
	std::uint32_t section_alignment) const noexcept
{
	if (!std::has_single_bit(section_alignment))
		return false;
	return (base_struct()->virtual_address % section_alignment) == 0;
}

bool section_header::check_low_alignment() const noexcept
{
	return base_struct()->virtual_address == get_pointer_to_raw_data();
}

std::uint32_t section_header::get_pointer_to_raw_data() const noexcept
{
	auto result = base_struct()->pointer_to_raw_data;
	if (result <= max_raw_address_rounded_to_0)
		result = 0;
	return result;
}

section_header& section_header::set_pointer_to_raw_data(
	std::uint32_t pointer_to_raw_data) noexcept
{
	base_struct()->pointer_to_raw_data = pointer_to_raw_data;
	return *this;
}

section_header::characteristics::value
	section_header::get_characteristics() const noexcept
{
	return static_cast<characteristics::value>(
		base_struct()->characteristics);
}

section_header& section_header::set_characteristics(
	characteristics::value value) noexcept
{
	base_struct()->characteristics = value;
	return *this;
}

bool section_header::is_writable() const noexcept
{
	return static_cast<bool>(get_characteristics()
		& characteristics::mem_write);
}

bool section_header::is_readable() const noexcept
{
	return static_cast<bool>(get_characteristics()
		& characteristics::mem_read);
}

bool section_header::is_executable() const noexcept
{
	return static_cast<bool>(get_characteristics()
		& characteristics::mem_execute);
}

bool section_header::is_shared() const noexcept
{
	return static_cast<bool>(get_characteristics()
		& characteristics::mem_shared);
}

bool section_header::is_discardable() const noexcept
{
	return static_cast<bool>(get_characteristics()
		& characteristics::mem_discardable);
}

bool section_header::is_pageable() const noexcept
{
	return !static_cast<bool>(get_characteristics()
		& characteristics::mem_not_paged);
}

bool section_header::is_cacheable() const noexcept
{
	return !static_cast<bool>(get_characteristics()
		& characteristics::mem_not_cached);
}

void section_header::toggle_characteristic(bool toggle,
	std::uint32_t flag) noexcept
{
	base_struct()->characteristics
		^= ((0u - toggle) ^ base_struct()->characteristics) & flag;
}

section_header& section_header::set_writable(bool writable) noexcept
{
	toggle_characteristic(writable, characteristics::mem_write);
	return *this;
}

section_header& section_header::set_readable(bool readable) noexcept
{
	toggle_characteristic(readable, characteristics::mem_read);
	return *this;
}

section_header& section_header::set_executable(bool executable) noexcept
{
	toggle_characteristic(executable, characteristics::mem_execute);
	return *this;
}

section_header& section_header::set_shared(bool shared) noexcept
{
	toggle_characteristic(shared, characteristics::mem_shared);
	return *this;
}

section_header& section_header::set_discardable(bool discardable) noexcept
{
	toggle_characteristic(discardable, characteristics::mem_discardable);
	return *this;
}

section_header& section_header::set_non_pageable(bool non_pageable) noexcept
{
	toggle_characteristic(non_pageable, characteristics::mem_not_paged);
	return *this;
}

section_header& section_header::set_non_cacheable(bool non_cacheable) noexcept
{
	toggle_characteristic(non_cacheable, characteristics::mem_not_cached);
	return *this;
}

bool section_header::empty() const noexcept
{
	return base_struct()->size_of_raw_data == 0;
}

section_header& section_header::set_virtual_size(
	std::uint32_t virtual_size)
{
	if (!virtual_size)
	{
		if (empty())
			throw pe_error(section_table_errc::invalid_section_virtual_size);

		base_struct()->virtual_size = base_struct()->size_of_raw_data;
	}
	else
	{
		base_struct()->virtual_size = virtual_size;
	}
	return *this;
}

section_header& section_header::set_raw_size(std::uint32_t raw_size) noexcept
{
	base_struct()->size_of_raw_data = raw_size;
	return *this;
}

} //namespace pe_bliss
