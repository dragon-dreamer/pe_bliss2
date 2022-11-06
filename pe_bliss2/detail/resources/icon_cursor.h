#pragma once

#include <cstdint>

namespace pe_bliss::detail::resources
{

//ico file header
struct ico_header
{
	std::uint16_t reserved; //zeros
	std::uint16_t type; //1
	std::uint16_t count; //Count of icons included in icon group
};

//structure that is stored in icon group directory in PE resources
struct icon_group
{
	std::uint8_t width;
	std::uint8_t height;
	std::uint8_t color_count;
	std::uint8_t reserved;
	std::uint16_t planes;
	std::uint16_t bit_count;
	std::uint32_t size_in_bytes;
	std::uint16_t number; //resource ID in PE icon list
};

//icon directory entry inside ico file
struct icondirentry
{
	std::uint8_t width;
	std::uint8_t height;
	std::uint8_t color_count;
	std::uint8_t reserved;
	std::uint16_t planes;
	std::uint16_t bit_count;
	std::uint32_t size_in_bytes;
	std::uint32_t image_offset; //offset from start of header to the image
};

//cur file header
struct cursor_header
{
	std::uint16_t reserved; //zeros
	std::uint16_t type; //2
	std::uint16_t count; //count of cursors included in cursor group
};

//structure that is stored in cursor group directory in PE resources
struct cursor_group
{
	std::uint16_t width;
	std::uint16_t height; //divide by 2 to get the actual height.
	std::uint16_t planes;
	std::uint16_t bit_count;
	std::uint32_t size_in_bytes;
	std::uint16_t number; //resource ID in PE cursor list
};

//cursor directory entry inside cur file
struct cursordirentry
{
	std::uint8_t width; 
	std::uint8_t height; //set to cursor_group::height / 2.
	std::uint8_t color_count;
	std::uint8_t reserved;
	std::uint16_t hotspot_x;
	std::uint16_t hotspot_y;
	std::uint32_t size_in_bytes;
	std::uint32_t image_offset; //offset from start of header to the image
};

struct cursor_hotspots
{
	std::uint16_t hotspot_x;
	std::uint16_t hotspot_y;
};

constexpr std::uint16_t icon_type = 1u;
constexpr std::uint16_t cursor_type = 2u;

} //namespace pe_bliss::detail::resources
