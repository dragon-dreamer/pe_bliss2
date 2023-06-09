#pragma once

#include <array>
#include <cstdint>

namespace pe_bliss::detail
{

struct image_dos_header
{
	std::uint16_t e_magic;
	std::uint16_t e_cblp;
	std::uint16_t e_cp;
	std::uint16_t e_crlc;
	std::uint16_t e_cparhdr;
	std::uint16_t e_minalloc;
	std::uint16_t e_maxalloc;
	std::uint16_t e_ss;
	std::uint16_t e_sp;
	std::uint16_t e_csum;
	std::uint16_t e_ip;
	std::uint16_t e_cs;
	std::uint16_t e_lfarlc;
	std::uint16_t e_ovno;
	std::array<std::uint16_t, 4u> e_res;
	std::uint16_t e_oemid;
	std::uint16_t e_oeminfo;
	std::array<std::uint16_t, 10u> e_res2;
	std::uint32_t e_lfanew;
};

} //namespace pe_bliss::detail
