#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "buffers/input_buffer_interface.h"

#include "pe_bliss2/pe_types.h"

namespace pe_bliss::image
{

class image;

[[nodiscard]]
buffers::input_buffer_ptr section_data_from_rva(const image& instance,
	rva_type rva, std::uint32_t data_size, bool include_headers = false,
	bool allow_virtual_data = false);
[[nodiscard]]
std::span<std::byte> section_data_from_rva(image& instance,
	rva_type rva, std::uint32_t data_size, bool include_headers = false,
	bool allow_virtual_data = false);
[[nodiscard]]
buffers::input_buffer_ptr section_data_from_va(const image& instance,
	std::uint32_t va, std::uint32_t data_size, bool include_headers = false,
	bool allow_virtual_data = false);
[[nodiscard]]
std::span<std::byte> section_data_from_va(image& instance,
	std::uint32_t va, std::uint32_t data_size, bool include_headers = false,
	bool allow_virtual_data = false);
[[nodiscard]]
buffers::input_buffer_ptr section_data_from_va(const image& instance,
	std::uint64_t va, std::uint32_t data_size, bool include_headers = false,
	bool allow_virtual_data = false);
[[nodiscard]]
std::span<std::byte> section_data_from_va(image& instance,
	std::uint64_t va, std::uint32_t data_size, bool include_headers = false,
	bool allow_virtual_data = false);

[[nodiscard]]
buffers::input_buffer_ptr section_data_from_rva(const image& instance,
	rva_type rva, bool include_headers = false);
[[nodiscard]]
std::span<std::byte> section_data_from_rva(image& instance,
	rva_type rva, bool include_headers = false);
[[nodiscard]]
buffers::input_buffer_ptr section_data_from_va(const image& instance,
	std::uint32_t va, bool include_headers = false);
[[nodiscard]]
std::span<std::byte> section_data_from_va(image& instance,
	std::uint32_t va, bool include_headers = false);
[[nodiscard]]
buffers::input_buffer_ptr section_data_from_va(const image& instance,
	std::uint64_t va, bool include_headers = false);
[[nodiscard]]
std::span<std::byte> section_data_from_va(image& instance,
	std::uint64_t va, bool include_headers = false);

} //namespace pe_bliss::image
