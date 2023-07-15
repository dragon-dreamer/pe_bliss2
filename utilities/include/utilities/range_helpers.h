#pragma once

#include <algorithm>
#include <cstddef>
#include <utility>

#include "utilities/hash.h"

namespace utilities
{

struct range_hash
{
	template<typename T>
	auto operator()(const T& range) const noexcept
	{
		std::size_t hash{};
		for (const auto& elem : range)
			utilities::hash_combine(hash, std::hash<typename T::value_type>{}(elem));

		return hash;
	}

	using is_transparent = void;
};

struct range_equal
{
	template<typename T1, typename T2>
	constexpr bool operator()(const T1& range_l, const T2& range_r) const noexcept
	{
		return std::ranges::equal(range_l, range_r);
	}

	using is_transparent = void;
};

} //namespace utilities
