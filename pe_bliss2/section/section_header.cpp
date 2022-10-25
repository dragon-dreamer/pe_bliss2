#include "pe_bliss2/section/section_header.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <system_error>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/section/section_errc.h"
#include "pe_bliss2/section/section_table.h"
#include "utilities/math.h"

namespace pe_bliss::section
{

void section_header::deserialize(buffers::input_buffer_stateful_wrapper_ref& buf,
	bool allow_virtual_data)
{
	try
	{
		get_descriptor().deserialize(buf, allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		std::throw_with_nested(pe_error(
			section_errc::unable_to_read_section_table));
	}
}

void section_header::serialize(buffers::output_buffer_interface& buf,
	bool write_virtual_part) const
{
	get_descriptor().serialize(buf, write_virtual_part);
}

rva_type section_header::rva_from_section_offset(
	std::uint32_t raw_offset_from_section_start,
	std::uint32_t section_alignment) const
{
	if (raw_offset_from_section_start >= get_virtual_size(section_alignment))
		throw pe_error(section_errc::invalid_section_offset);

	return get_rva() + raw_offset_from_section_start;
}

std::string_view section_header::get_name() const noexcept
{
	auto begin = get_descriptor()->name;
	auto end = begin + sizeof(detail::image_section_header::name) - 1;
	while (end >= begin && !*end)
		--end;

	return { reinterpret_cast<const char*>(get_descriptor()->name),
		static_cast<std::size_t>(end - begin + 1) };
}

section_header& section_header::set_name(std::string_view name)
{
	if (name.size() > sizeof(detail::image_section_header::name))
		throw pe_error(utilities::generic_errc::buffer_overrun);

	std::fill(std::begin(get_descriptor()->name),
		std::end(get_descriptor()->name), static_cast<std::uint8_t>(0u));
	std::copy(std::begin(name), std::end(name), get_descriptor()->name);
	return *this;
}

rva_type section_header::get_rva() const noexcept
{
	return get_descriptor()->virtual_address;
}

section_header& section_header::set_rva(std::uint32_t rva) noexcept
{
	get_descriptor()->virtual_address = rva;
	return *this;
}

std::uint32_t section_header::get_raw_size(
	std::uint32_t section_alignment) const noexcept
{
	auto raw_size = get_descriptor()->size_of_raw_data;
	auto virtual_size = get_virtual_size(section_alignment);
	if (raw_size > virtual_size)
		raw_size = virtual_size;
	return raw_size;
}

std::uint32_t section_header::get_virtual_size(
	std::uint32_t section_alignment) const noexcept
{
	auto virtual_size = get_descriptor()->virtual_size;
	if (!virtual_size)
		virtual_size = get_descriptor()->size_of_raw_data;
	if (!std::has_single_bit(section_alignment))
		return virtual_size;

	(void)utilities::math::align_up_if_safe(
		virtual_size, section_alignment);

	return virtual_size;
}

bool section_header::is_low_alignment() const noexcept
{
	return get_descriptor()->virtual_address == get_pointer_to_raw_data();
}

std::uint32_t section_header::get_pointer_to_raw_data() const noexcept
{
	auto result = get_descriptor()->pointer_to_raw_data;
	if (result <= max_raw_address_rounded_to_0)
		result = 0;
	return result;
}

section_header& section_header::set_pointer_to_raw_data(
	std::uint32_t pointer_to_raw_data) noexcept
{
	get_descriptor()->pointer_to_raw_data = pointer_to_raw_data;
	return *this;
}

section_header::characteristics::value
	section_header::get_characteristics() const noexcept
{
	return static_cast<characteristics::value>(
		get_descriptor()->characteristics);
}

section_header& section_header::set_characteristics(
	characteristics::value value) noexcept
{
	get_descriptor()->characteristics = value;
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
	get_descriptor()->characteristics
		^= ((0u - toggle) ^ get_descriptor()->characteristics) & flag;
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
	return get_descriptor()->size_of_raw_data == 0;
}

section_header& section_header::set_virtual_size(
	std::uint32_t virtual_size)
{
	if (!virtual_size)
	{
		if (empty())
			throw pe_error(section_errc::invalid_section_virtual_size);

		get_descriptor()->virtual_size = get_descriptor()->size_of_raw_data;
	}
	else
	{
		get_descriptor()->virtual_size = virtual_size;
	}
	return *this;
}

section_header& section_header::set_raw_size(std::uint32_t raw_size) noexcept
{
	get_descriptor()->size_of_raw_data = raw_size;
	return *this;
}

} //namespace pe_bliss::section
