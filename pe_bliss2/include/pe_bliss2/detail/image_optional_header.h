#pragma once

#include <cstdint>

namespace pe_bliss::detail
{

struct image_optional_header_32
{
	//magic is excluded and parsed separately, because other structure fields depend on it
	/* std::uint16_t magic; */
	std::uint8_t major_linker_version;
	std::uint8_t minor_linker_version;
	std::uint32_t size_of_code;
	std::uint32_t size_of_initialized_data;
	std::uint32_t size_of_uninitialized_data;
	std::uint32_t address_of_entry_point;
	std::uint32_t base_of_code;
	std::uint32_t base_of_data;
	std::uint32_t image_base;
	std::uint32_t section_alignment;
	std::uint32_t file_alignment;
	std::uint16_t major_operating_system_version;
	std::uint16_t minor_operating_system_version;
	std::uint16_t major_image_version;
	std::uint16_t minor_image_version;
	std::uint16_t major_subsystem_version;
	std::uint16_t minor_subsystem_version;
	std::uint32_t win32_version_value;
	std::uint32_t size_of_image;
	std::uint32_t size_of_headers;
	std::uint32_t checksum;
	std::uint16_t subsystem;
	std::uint16_t dll_characteristics;
	std::uint32_t size_of_stack_reserve;
	std::uint32_t size_of_stack_commit;
	std::uint32_t size_of_heap_reserve;
	std::uint32_t size_of_heap_commit;
	std::uint32_t loader_flags;
	std::uint32_t number_of_rva_and_sizes;
};

struct image_optional_header_64
{
	//magic is excluded and parsed separately, because other structure fields depend on it
	/* std::uint16_t magic; */
	std::uint8_t major_linker_version;
	std::uint8_t minor_linker_version;
	std::uint32_t size_of_code;
	std::uint32_t size_of_initialized_data;
	std::uint32_t size_of_uninitialized_data;
	std::uint32_t address_of_entry_point;
	std::uint32_t base_of_code;
	std::uint64_t image_base;
	std::uint32_t section_alignment;
	std::uint32_t file_alignment;
	std::uint16_t major_operating_system_version;
	std::uint16_t minor_operating_system_version;
	std::uint16_t major_image_version;
	std::uint16_t minor_image_version;
	std::uint16_t major_subsystem_version;
	std::uint16_t minor_subsystem_version;
	std::uint32_t win32_version_value;
	std::uint32_t size_of_image;
	std::uint32_t size_of_headers;
	std::uint32_t checksum;
	std::uint16_t subsystem;
	std::uint16_t dll_characteristics;
	std::uint64_t size_of_stack_reserve;
	std::uint64_t size_of_stack_commit;
	std::uint64_t size_of_heap_reserve;
	std::uint64_t size_of_heap_commit;
	std::uint32_t loader_flags;
	std::uint32_t number_of_rva_and_sizes;
};

} //namespace pe_bliss::detail
