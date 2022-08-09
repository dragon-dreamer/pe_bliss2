#pragma once

#include <cstdint>
#include <string_view>

#include "pe_bliss2/detail/image_section_header.h"
#include "pe_bliss2/detail/packed_struct_base.h"
#include "pe_bliss2/pe_types.h"

#include "utilities/static_class.h"

namespace buffers
{
class input_buffer_stateful_wrapper_ref;
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::section
{

class [[nodiscard]] section_header
	: public detail::packed_struct_base<detail::image_section_header>
{
public:
	struct characteristics final : utilities::static_class
	{
		enum value : std::uint32_t
		{
			mem_discardable = 0x02000000,
			mem_not_cached = 0x04000000,
			mem_not_paged = 0x08000000,
			mem_shared = 0x10000000,
			mem_execute = 0x20000000,
			mem_read = 0x40000000,
			mem_write = 0x80000000,
			cnt_code = 0x00000020,
			cnt_initialized_data = 0x00000040,
			cnt_uninitialized_data = 0x00000080
		};
	};

public:
	static constexpr std::uint32_t max_raw_address_rounded_to_0 = 0x1ffu;
	static constexpr std::uint32_t two_gb_size = 1024u * 1024u * 1024u * 2u;

public:
	//When deserializing, buf should point to the image_section_header structure
	void deserialize(buffers::input_buffer_stateful_wrapper_ref& buf,
		bool allow_virtual_memory = false);
	void serialize(buffers::output_buffer_interface& buf,
		bool write_virtual_part = true) const;

public:
	[[nodiscard]]
	std::string_view get_name() const noexcept;
	section_header& set_name(std::string_view name);

	[[nodiscard]]
	characteristics::value get_characteristics() const noexcept;

	[[nodiscard]]
	std::uint32_t get_raw_size(std::uint32_t section_alignment) const noexcept;
	[[nodiscard]]
	std::uint32_t get_virtual_size(std::uint32_t section_alignment) const noexcept;
	[[nodiscard]]
	std::uint32_t get_pointer_to_raw_data() const noexcept;
	[[nodiscard]]
	rva_type get_rva() const noexcept;
	[[nodiscard]]
	bool empty() const noexcept;

public:
	[[nodiscard]]
	bool is_low_alignment() const noexcept;

public:
	[[nodiscard]] bool is_writable() const noexcept;
	[[nodiscard]] bool is_readable() const noexcept;
	[[nodiscard]] bool is_executable() const noexcept;
	[[nodiscard]] bool is_shared() const noexcept;
	[[nodiscard]] bool is_discardable() const noexcept;
	[[nodiscard]] bool is_pageable() const noexcept;
	[[nodiscard]] bool is_cacheable() const noexcept;

public:
	section_header& set_writable(bool writable) noexcept;
	section_header& set_readable(bool readable) noexcept;
	section_header& set_executable(bool executable) noexcept;
	section_header& set_shared(bool shared) noexcept;
	section_header& set_discardable(bool discardable) noexcept;
	section_header& set_non_pageable(bool non_pageable) noexcept;
	section_header& set_non_cacheable(bool non_cacheable) noexcept;
	section_header& set_characteristics(characteristics::value value) noexcept;

public:
	section_header& set_virtual_size(std::uint32_t virtual_size);
	section_header& set_raw_size(std::uint32_t raw_size) noexcept;
	section_header& set_pointer_to_raw_data(
		std::uint32_t pointer_to_raw_data) noexcept;
	section_header& set_rva(std::uint32_t rva) noexcept;

public:
	[[nodiscard]]
	rva_type rva_from_section_offset(std::uint32_t raw_offset_from_section_start,
		std::uint32_t section_alignment) const;

private:
	void toggle_characteristic(bool toggle, std::uint32_t flag) noexcept;
};

} //namespace pe_bliss::section
