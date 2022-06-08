#pragma once

#include <cstdint>
#include <vector>

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

struct test_section_info
{
	std::uint32_t virtual_size = 0x1000u;
	std::uint32_t raw_size = 0x1000u;
};

struct test_image_options
{
	bool is_x64 = false;
	std::uint32_t number_of_data_directories = 16u;
	std::uint32_t image_base = 0x40000u;
	std::uint32_t file_alignment = 0x200u;
	std::uint32_t section_alignment = 0x1000u;
	std::uint32_t e_lfanew = 0xf8u;
	std::uint32_t start_section_rva = 0x1000u;
	std::uint32_t start_section_raw_offset = 0x1000u;
	std::vector<test_section_info> sections{
		{ 0x1000u, 0x1000u },
		{ 0x2000u, 0x1000u },
		{ 0x3000u, 0x0u },
	};
};

[[nodiscard]]
pe_bliss::image::image create_test_image(const test_image_options& options);
