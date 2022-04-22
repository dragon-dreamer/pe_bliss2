#pragma once

#include <cstdint>

namespace pe_bliss::detail::resources
{

struct image_resource_directory
{
	std::uint32_t characteristics;
	std::uint32_t time_date_stamp;
	std::uint16_t major_version;
	std::uint16_t minor_version;
	std::uint16_t number_of_named_entries;
	std::uint16_t number_of_id_entries;
	//image_resource_directory_entry directory_entries[];
};

struct image_resource_directory_entry
{
	std::uint32_t name_or_id;
	std::uint32_t offset_to_data_or_directory;
};

constexpr std::uint32_t name_is_string_flag = 0x80000000u;
constexpr std::uint32_t data_is_directory_flag = 0x80000000u;

struct image_resource_data_entry
{
	std::uint32_t offset_to_data;
	std::uint32_t size;
	std::uint32_t code_page;
	std::uint32_t reserved;
};

} //namespace pe_bliss::detail::resources
