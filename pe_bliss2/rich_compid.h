#pragma once

#include <cstdint>

namespace pe_bliss
{

class rich_compid
{
public:
	std::uint16_t build_number{};
	std::uint16_t prod_id{};
	std::uint32_t use_count{};
};

} //namespace pe_bliss
