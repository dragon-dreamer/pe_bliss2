#pragma once

#include <cstdint>
#include <list>

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/pe_types.h"
#include "pe_bliss2/section/section_header.h"

namespace buffers
{
class input_buffer_interface;
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::core
{
class optional_header;
} //namespace pe_bliss::core

namespace pe_bliss::section
{

class [[nodiscard]] section_table
{
public:
	using header_list = std::list<section_header>;

public:
	//When deserializing, buf should point to
	//the section table start
	void deserialize(buffers::input_buffer_interface& buf,
		std::uint16_t number_of_sections, bool allow_virtual_memory = false);
	//When deserializing, buf should point to
	//the section table start
	void serialize(buffers::output_buffer_interface& buf,
		bool write_virtual_part = true) const;

	[[nodiscard]]
	pe_error_wrapper validate_section_header(const core::optional_header& oh,
		header_list::const_iterator header) const noexcept;
	[[nodiscard]]
	pe_error_wrapper validate_section_headers(
		const core::optional_header& oh) const noexcept;
	[[nodiscard]]
	pe_error_wrapper validate_size_of_image(
		const core::optional_header& oh) const noexcept;

	[[nodiscard]]
	header_list& get_section_headers() & noexcept
	{
		return headers_;
	}

	[[nodiscard]]
	header_list get_section_headers() && noexcept
	{
		auto result = std::move(headers_);
		headers_.clear();
		return result;
	}

	[[nodiscard]]
	const header_list& get_section_headers() const & noexcept
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
	header_list::const_iterator by_rva(rva_type rva,
		std::uint32_t section_alignment,
		std::uint32_t data_size = 0) const;
	[[nodiscard]]
	header_list::iterator by_rva(rva_type rva,
		std::uint32_t section_alignment,
		std::uint32_t data_size = 0);
	[[nodiscard]]
	header_list::const_iterator by_raw_offset(std::uint32_t raw_offset,
		std::uint32_t section_alignment, std::uint32_t data_size = 0) const;
	[[nodiscard]]
	header_list::iterator by_raw_offset(std::uint32_t raw_offset,
		std::uint32_t section_alignment, std::uint32_t data_size = 0);
	[[nodiscard]]
	header_list::const_iterator by_reference(
		const section_header& header) const noexcept;
	[[nodiscard]]
	header_list::iterator by_reference(section_header& header) noexcept;

private:
	header_list headers_;
	std::size_t buffer_pos_ = 0;
};

} //namespace pe_bliss::section
