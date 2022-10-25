#include "pe_bliss2/core/optional_header.h"

#include <algorithm>
#include <exception>
#include <system_error>
#include <variant>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/core/optional_header_errc.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/variant_helpers.h"

namespace pe_bliss::core
{

void optional_header::deserialize(buffers::input_buffer_stateful_wrapper_ref& buf,
	bool allow_virtual_data)
{
	packed_struct<std::uint16_t> magic_value;
	try
	{
		magic_value.deserialize(buf, false);
	}
	catch (const std::system_error&)
	{
		std::throw_with_nested(pe_error(
			optional_header_errc::unable_to_read_optional_header));
	}

	if (magic_value.get() == static_cast<uint16_t>(magic::pe32))
		initialize_with<optional_header_32_type>();
	else if (magic_value.get() == static_cast<uint16_t>(magic::pe64))
		initialize_with<optional_header_64_type>();
	else
		throw pe_error(optional_header_errc::invalid_pe_magic);

	try
	{
		std::visit([&buf, allow_virtual_data](auto& obj) {
			obj.deserialize(buf, allow_virtual_data);
		}, header_);
	}
	catch (const std::system_error&)
	{
		std::throw_with_nested(pe_error(
			optional_header_errc::unable_to_read_optional_header));
	}
}

void optional_header::serialize(buffers::output_buffer_interface& buf,
	bool write_virtual_part) const
{
	packed_struct magic_value(static_cast<magic_type>(get_magic()));
	magic_value.serialize(buf, true);
	std::visit([&buf, write_virtual_part] (const auto& obj) {
		obj.serialize(buf, write_virtual_part);
	}, header_);
}

std::uint32_t optional_header::get_number_of_rva_and_sizes() const noexcept
{
	return (std::min)(access([] (auto& obj) {
		return obj.number_of_rva_and_sizes; }),
		max_number_of_rva_and_sizes);
}

std::uint32_t optional_header::get_size_of_structure() const noexcept
{
	return sizeof(magic_type) + access([] (const auto& value) {
		return static_cast<std::uint32_t>(
			detail::packed_reflection::get_type_size<
				std::remove_cvref_t<decltype(value)>>());
	});
}

bool optional_header::is_low_alignment() const noexcept
{
	auto file_alignment = get_raw_file_alignment();
	return file_alignment == get_raw_section_alignment()
		&& file_alignment >= 1 && file_alignment <= 0x800;
}

std::uint8_t optional_header::get_raw_major_linker_version() const noexcept
{
	return access([] (const auto& obj) { return obj.major_linker_version; });
}

std::uint8_t optional_header::get_raw_minor_linker_version() const noexcept
{
	return access([] (const auto& obj) { return obj.minor_linker_version; });
}

std::uint32_t optional_header::get_raw_size_of_code() const noexcept
{
	return access([] (const auto& obj) { return obj.size_of_code; });
}

std::uint32_t optional_header::get_raw_size_of_initialized_data() const noexcept
{
	return access([] (const auto& obj) { return obj.size_of_initialized_data; });
}

std::uint32_t optional_header::get_raw_size_of_uninitialized_data() const noexcept
{
	return access([] (const auto& obj) { return obj.size_of_uninitialized_data; });
}

rva_type optional_header::get_raw_address_of_entry_point() const noexcept
{
	return access([] (const auto& obj) { return obj.address_of_entry_point; });
}

std::uint32_t optional_header::get_raw_base_of_code() const noexcept
{
	return access([] (const auto& obj) { return obj.base_of_code; });
}

std::uint32_t optional_header::get_raw_base_of_data() const
{
	return access(utilities::overloaded{
		[] (const detail::image_optional_header_32& obj) { return obj.base_of_data; },
		[] (const detail::image_optional_header_64&) -> std::uint32_t {
			throw pe_error(optional_header_errc::no_base_of_data_field);
		},
	});
}

std::uint64_t optional_header::get_raw_image_base() const noexcept
{
	return access([] (const auto& obj) {
		return static_cast<std::uint64_t>(obj.image_base); });
}

std::uint32_t optional_header::get_raw_section_alignment() const noexcept
{
	return access([] (const auto& obj) { return obj.section_alignment; });
}

std::uint32_t optional_header::get_raw_file_alignment() const noexcept
{
	return access([] (const auto& obj) { return obj.file_alignment; });
}

std::uint16_t optional_header::get_raw_major_operating_system_version() const noexcept
{
	return access([] (const auto& obj) {
		return obj.major_operating_system_version; });
}

std::uint16_t optional_header::get_raw_minor_operating_system_version() const noexcept
{
	return access([] (const auto& obj) {
		return obj.minor_operating_system_version; });
}

std::uint16_t optional_header::get_raw_major_image_version() const noexcept
{
	return access([] (const auto& obj) { return obj.major_image_version; });
}

std::uint16_t optional_header::get_raw_minor_image_version() const noexcept
{
	return access([] (const auto& obj) { return obj.minor_image_version; });
}

std::uint16_t optional_header::get_raw_major_subsystem_version() const noexcept
{
	return access([] (const auto& obj) { return obj.major_subsystem_version; });
}

std::uint16_t optional_header::get_raw_minor_subsystem_version() const noexcept
{
	return access([] (const auto& obj) { return obj.minor_subsystem_version; });
}

std::uint32_t optional_header::get_raw_win32_version_value() const noexcept
{
	return access([] (const auto& obj) { return obj.win32_version_value; });
}

std::uint32_t optional_header::get_raw_size_of_image() const noexcept
{
	return access([] (const auto& obj) { return obj.size_of_image; });
}

std::uint32_t optional_header::get_raw_size_of_headers() const noexcept
{
	return access([] (const auto& obj) { return obj.size_of_headers; });
}

std::uint32_t optional_header::get_raw_checksum() const noexcept
{
	return access([] (const auto& obj) { return obj.checksum; });
}

std::uint16_t optional_header::get_raw_subsystem() const noexcept
{
	return access([] (const auto& obj) { return obj.subsystem; });
}

std::uint16_t optional_header::get_raw_dll_characteristics() const noexcept
{
	return access([] (const auto& obj) { return obj.dll_characteristics; });
}

std::uint64_t optional_header::get_raw_size_of_stack_reserve() const noexcept
{
	return access([] (const auto& obj) {
		return static_cast<std::uint64_t>(obj.size_of_stack_reserve); });
}

std::uint64_t optional_header::get_raw_size_of_stack_commit() const noexcept
{
	return access([] (const auto& obj) {
		return static_cast<std::uint64_t>(obj.size_of_stack_commit); });
}

std::uint64_t optional_header::get_raw_size_of_heap_reserve() const noexcept
{
	return access([] (const auto& obj) {
		return static_cast<std::uint64_t>(obj.size_of_heap_reserve); });
}

std::uint64_t optional_header::get_raw_size_of_heap_commit() const noexcept
{
	return access([] (const auto& obj) {
		return static_cast<std::uint64_t>(obj.size_of_heap_commit); });
}

std::uint32_t optional_header::get_raw_loader_flags() const noexcept
{
	return access([] (const auto& obj) { return obj.loader_flags; });
}

std::uint32_t optional_header::get_raw_number_of_rva_and_sizes() const noexcept
{
	return access([] (const auto& obj) { return obj.number_of_rva_and_sizes; });
}

void optional_header::set_raw_major_linker_version(std::uint8_t version) noexcept
{
	access([version] (auto& obj) { obj.major_linker_version = version; });
}

void optional_header::set_raw_minor_linker_version(std::uint8_t version) noexcept
{
	access([version] (auto& obj) { obj.minor_linker_version = version; });
}

void optional_header::set_raw_size_of_code(std::uint32_t size) noexcept
{
	access([size] (auto& obj) { obj.size_of_code = size; });
}

void optional_header::set_raw_size_of_initialized_data(
	std::uint32_t size) noexcept
{
	access([size] (auto& obj) {
		obj.size_of_initialized_data = size; });
}

void optional_header::set_raw_size_of_uninitialized_data(
	std::uint32_t size) noexcept
{
	access([size] (auto& obj) {
		obj.size_of_uninitialized_data = size; });
}

void optional_header::set_raw_address_of_entry_point(rva_type address) noexcept
{
	access([address] (auto& obj) { obj.address_of_entry_point = address; });
}

void optional_header::set_raw_base_of_code(std::uint32_t base) noexcept
{
	access([base] (auto& obj) { obj.base_of_code = base; });
}

void optional_header::set_raw_base_of_data(std::uint32_t base)
{
	access(utilities::overloaded{
		[base] (detail::image_optional_header_32& obj) { obj.base_of_data = base; },
		[] (detail::image_optional_header_64&) {
			throw pe_error(optional_header_errc::no_base_of_data_field);
		},
	});
}

void optional_header::set_raw_image_base(std::uint64_t base) noexcept
{
	access([base] (auto& obj) { obj.image_base =
		static_cast<decltype(obj.image_base)>(base); });
}

void optional_header::set_raw_section_alignment(std::uint32_t alignment) noexcept
{
	access([alignment] (auto& obj) { obj.section_alignment = alignment; });
}

void optional_header::set_raw_file_alignment(std::uint32_t alignment) noexcept
{
	access([alignment] (auto& obj) { obj.file_alignment = alignment; });
}

void optional_header::set_raw_major_operating_system_version(
	std::uint16_t version) noexcept
{
	access([version] (auto& obj) {
		obj.major_operating_system_version = version; });
}

void optional_header::set_raw_minor_operating_system_version(
	std::uint16_t version) noexcept
{
	access([version] (auto& obj) {
		obj.minor_operating_system_version = version; });
}

void optional_header::set_raw_major_image_version(std::uint16_t version) noexcept
{
	access([version] (auto& obj) { obj.major_image_version = version; });
}

void optional_header::set_raw_minor_image_version(std::uint16_t version) noexcept
{
	access([version] (auto& obj) { obj.minor_image_version = version; });
}

void optional_header::set_raw_major_subsystem_version(
	std::uint16_t version) noexcept
{
	access([version] (auto& obj) {
		obj.major_subsystem_version = version; });
}

void optional_header::set_raw_minor_subsystem_version(
	std::uint16_t version) noexcept
{
	access([version] (auto& obj) {
		obj.minor_subsystem_version = version; });
}

void optional_header::set_raw_win32_version_value(std::uint32_t version) noexcept
{
	access([version] (auto& obj) { obj.win32_version_value = version; });
}

void optional_header::set_raw_size_of_image(std::uint32_t size) noexcept
{
	access([size] (auto& obj) { obj.size_of_image = size; });
}

void optional_header::set_raw_size_of_headers(std::uint32_t size) noexcept
{
	access([size] (auto& obj) { obj.size_of_headers = size; });
}

void optional_header::set_raw_checksum(std::uint32_t checksum) noexcept
{
	access([checksum] (auto& obj) { obj.checksum = checksum; });
}

void optional_header::set_raw_subsystem(std::uint16_t subsystem) noexcept
{
	access([subsystem] (auto& obj) { obj.subsystem = subsystem; });
}

void optional_header::set_raw_dll_characteristics(
	std::uint16_t characteristics) noexcept
{
	access([characteristics] (auto& obj) {
		obj.dll_characteristics = characteristics; });
}

void optional_header::set_raw_size_of_stack_reserve(std::uint64_t size) noexcept
{
	access([size] (auto& obj) { obj.size_of_stack_reserve
		= static_cast<decltype(obj.size_of_stack_reserve)>(size); });
}

void optional_header::set_raw_size_of_stack_commit(std::uint64_t size) noexcept
{
	access([size] (auto& obj) { obj.size_of_stack_commit
		= static_cast<decltype(obj.size_of_stack_commit)>(size); });
}

void optional_header::set_raw_size_of_heap_reserve(std::uint64_t size) noexcept
{
	access([size] (auto& obj) { obj.size_of_heap_reserve
		= static_cast<decltype(obj.size_of_heap_reserve)>(size); });
}

void optional_header::set_raw_size_of_heap_commit(std::uint64_t size) noexcept
{
	access([size] (auto& obj) { obj.size_of_heap_commit
		= static_cast<decltype(obj.size_of_heap_commit)>(size); });
}

void optional_header::set_raw_loader_flags(std::uint32_t flags) noexcept
{
	access([flags] (auto& obj) { obj.loader_flags = flags; });
}

void optional_header::set_raw_number_of_rva_and_sizes(
	std::uint32_t number) noexcept
{
	access([number] (auto& obj) {
		obj.number_of_rva_and_sizes = number; });
}

} //namespace pe_bliss::core
