#pragma once

#include <cstdint>

namespace pe_bliss::detail::resources
{

struct message_resource_data
{
	std::uint32_t number_of_blocks;
	//message_resource_block blocks[1];
};

struct message_resource_block
{
	std::uint32_t low_id;
	std::uint32_t high_id;
	std::uint32_t offset_to_entries;
};

struct message_resource_entry
{
	std::uint16_t length;
	std::uint16_t flags;
	//std::byte text[1];
};

constexpr std::uint16_t message_resource_ansi = 0u;
constexpr std::uint16_t message_resource_unicode = 1u;
constexpr std::uint16_t message_resource_utf8 = 2u;

constexpr std::uint32_t error_severity_success = 0x00000000u;
constexpr std::uint32_t error_severity_informational = 0x40000000u;
constexpr std::uint32_t error_severity_warning = 0x80000000u;
constexpr std::uint32_t error_severity_error = 0xc0000000u;
constexpr std::uint32_t severity_mask = 0xc0000000u;
constexpr std::uint32_t facility_mask = 0xfff0000u;

} //namespace pe_bliss::detail::resources
