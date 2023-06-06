#pragma once

#include <cstddef>

namespace utilities
{

constexpr void hash_combine(std::size_t& seed, std::size_t hash) noexcept
{
	seed ^= hash + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
}

} //namespace utilities
