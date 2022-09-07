#pragma once

#include <cstdint>

namespace pe_bliss::detail::resources
{

struct bitmap_file_header
{
	std::uint16_t type;
	std::uint32_t size;
	std::uint16_t reserved1;
	std::uint16_t reserved2;
	std::uint32_t off_bits;
};

struct bitmap_info_header
{
	std::uint32_t size;
	std::int32_t width;
	std::int32_t height;
	std::uint16_t planes;
	std::uint16_t bit_count;
	std::uint32_t compression;
	std::uint32_t size_image;
	std::int32_t x_pels_per_meter;
	std::int32_t y_pels_per_meter;
	std::uint32_t clr_used;
	std::uint32_t clr_important;
};

constexpr std::uint16_t bm_signature = 0x4d42u; //"BM" signature

} //namespace pe_bliss::detail::resources
