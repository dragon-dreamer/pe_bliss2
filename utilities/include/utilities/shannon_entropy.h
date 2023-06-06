#pragma once

#include <array>
#include <cstddef>
#include <limits>

namespace utilities
{

class [[nodiscard]] shannon_entropy final
{
public:
	static constexpr float maximum_entropy = 8.f;

	constexpr void update(std::byte value) noexcept
	{
		++byte_count_[static_cast<std::uint8_t>(value)];
		++total_length_;
	}

	[[nodiscard]]
	float finalize() const noexcept;

private:
	std::array<std::size_t,
		1u << std::numeric_limits<std::uint8_t>::digits> byte_count_{};
	std::size_t total_length_{};
};

} //namespace utilities
