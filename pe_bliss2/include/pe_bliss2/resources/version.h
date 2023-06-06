#pragma once

#include <cstdint>

namespace pe_bliss::resources
{

struct [[nodiscard]] short_version
{
	std::uint16_t major{};
	std::uint16_t minor{};

	[[nodiscard]]
	friend constexpr auto operator<=>(const short_version&,
		const short_version&) = default;
};

struct [[nodiscard]] full_version
{
	std::uint16_t major{};
	std::uint16_t minor{};
	std::uint16_t build{};
	std::uint16_t revision{};

	[[nodiscard]]
	friend constexpr auto operator<=>(const full_version&,
		const full_version&) = default;
};

[[nodiscard]]
constexpr full_version version_from_components(std::uint32_t ms, std::uint32_t ls) noexcept
{
	return {
		.major = static_cast<std::uint16_t>(ms >> 16u),
		.minor = static_cast<std::uint16_t>(ms & 0xffffu),
		.build = static_cast<std::uint16_t>(ls >> 16u),
		.revision = static_cast<std::uint16_t>(ls & 0xffffu)
	};
}

} //namespace pe_bliss::resources
