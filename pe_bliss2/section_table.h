#pragma once

#include <list>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/pe_types.h"
#include "pe_bliss2/section_header.h"

namespace buffers
{
class input_buffer_interface;
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss
{

enum class section_table_errc
{
	invalid_section_raw_size = 1,
	invalid_section_virtual_size,
	invalid_section_raw_address,
	invalid_section_raw_size_alignment,
	invalid_section_raw_address_alignment,
	raw_section_size_overflow,
	virtual_section_size_overflow,
	invalid_section_virtual_address_alignment,
	virtual_gap_between_sections,
	invalid_section_low_alignment,
	virtual_gap_between_headers_and_first_section,
	invalid_size_of_image,
	unable_to_read_section_table
};

std::error_code make_error_code(section_table_errc) noexcept;

class file_header;
class optional_header;

class section_table
{
public:
	using header_list = std::list<section_header>;

public:
	//When deserializing, buf should point to the end of data directory structures
	void deserialize(buffers::input_buffer_interface& buf,
		const file_header& fh, const optional_header& oh,
		bool allow_virtual_memory = false);
	void serialize(buffers::output_buffer_interface& buf,
		bool write_virtual_part = true) const;

	[[nodiscard]]
	pe_error_wrapper validate_section_header(const optional_header& oh,
		header_list::const_iterator header) const noexcept;
	[[nodiscard]]
	pe_error_wrapper validate_section_headers(const optional_header& oh) const noexcept;
	[[nodiscard]]
	pe_error_wrapper validate_size_of_image(const optional_header& oh) const noexcept;

	[[nodiscard]]
	header_list& get_section_headers() noexcept
	{
		return headers_;
	}

	[[nodiscard]]
	const header_list& get_section_headers() const noexcept
	{
		return headers_;
	}

	[[nodiscard]] std::size_t buffer_pos() const noexcept
	{
		return buffer_pos_;
	}

	void set_buffer_pos(std::size_t buffer_pos) noexcept
	{
		buffer_pos_ = buffer_pos;
	}

public:
	[[nodiscard]]
	header_list::const_iterator by_rva(rva_type rva, std::uint32_t section_alignment,
		std::uint32_t data_size = 0) const;
	[[nodiscard]]
	header_list::iterator by_rva(rva_type rva, std::uint32_t section_alignment,
		std::uint32_t data_size = 0);
	[[nodiscard]]
	header_list::const_iterator by_raw_offset(std::uint32_t raw_offset,
		std::uint32_t section_alignment, std::uint32_t data_size = 0) const;
	[[nodiscard]]
	header_list::iterator by_raw_offset(std::uint32_t raw_offset,
		std::uint32_t section_alignment, std::uint32_t data_size = 0);
	[[nodiscard]]
	header_list::const_iterator by_reference(const section_header& header) const noexcept;
	[[nodiscard]]
	header_list::iterator by_reference(section_header& header) noexcept;

private:
	header_list headers_;
	std::size_t buffer_pos_ = 0;
};

} //namespace pe_bliss

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::section_table_errc> : true_type {};
} //namespace std
