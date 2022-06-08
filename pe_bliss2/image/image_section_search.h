#pragma once

#include <cstdint>
#include <utility>

#include "pe_bliss2/section/section_table.h"
#include "pe_bliss2/section/section_data.h"
#include "pe_bliss2/core/data_directories.h"

namespace pe_bliss::image
{

class image;

using section_ref = std::pair<
	section::section_table::header_list::iterator,
	section::section_data_list::iterator>;
using section_const_ref = std::pair<
	section::section_table::header_list::const_iterator,
	section::section_data_list::const_iterator>;

[[nodiscard]]
section_ref section_from_reference(image& instance,
	section::section_header& section_hdr) noexcept;
[[nodiscard]]
section_const_ref section_from_reference(const image& instance,
	const section::section_header& section_hdr) noexcept;

[[nodiscard]]
section_ref section_from_rva(image& instance, rva_type rva,
	std::uint32_t data_size = 0) noexcept;
[[nodiscard]]
section_const_ref section_from_rva(
	const image& instance, rva_type rva,
	std::uint32_t data_size = 0) noexcept;

[[nodiscard]]
section_ref section_from_va(image& instance, std::uint32_t va,
	std::uint32_t data_size = 0) noexcept;
[[nodiscard]]
section_const_ref section_from_va(
	const image& instance, std::uint32_t va,
	std::uint32_t data_size = 0) noexcept;
[[nodiscard]]
section_ref section_from_va(image& instance, std::uint64_t va,
	std::uint32_t data_size = 0) noexcept;
[[nodiscard]]
section_const_ref section_from_va(
	const image& instance, std::uint64_t va,
	std::uint32_t data_size = 0) noexcept;

[[nodiscard]]
section_ref section_from_directory(image& instance,
	core::data_directories::directory_type directory) noexcept;
[[nodiscard]]
section_const_ref section_from_directory(const image& instance,
	core::data_directories::directory_type directory) noexcept;

[[nodiscard]]
section_ref section_from_file_offset(
	image& instance, std::uint32_t offset,
	std::uint32_t data_size = 0) noexcept;
[[nodiscard]]
section_const_ref section_from_file_offset(
	const image& instance, std::uint32_t offset,
	std::uint32_t data_size = 0) noexcept;

} //namespace pe_bliss::image
