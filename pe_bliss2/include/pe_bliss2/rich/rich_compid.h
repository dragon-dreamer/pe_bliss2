#pragma once

#include <cstdint>
#include <compare>

namespace pe_bliss::rich
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

	[[nodiscard]]
	friend bool operator==(const rich_compid&,
		const rich_compid&) noexcept = default;
};

[[nodiscard]]
std::uint32_t get_checksum(const rich_compid& compid) noexcept;

} //namespace pe_bliss::rich
