#pragma once

#include <cstdint>
#include <compare>

namespace pe_bliss
{

class [[nodiscard]] rich_compid
{
public:
	std::uint16_t build_number{};
	std::uint16_t prod_id{};
	std::uint32_t use_count{};

	[[nodiscard]]
	friend auto operator<=>(const rich_compid&,
		const rich_compid&) noexcept = default;
};

} //namespace pe_bliss
