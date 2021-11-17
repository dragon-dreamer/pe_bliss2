#pragma once

#include <concepts>
#include <cstdint>
#include <limits>

#include "utilities/static_class.h"

namespace utilities
{

class math : public static_class
{
public:
	/** Checks if value is aligned on specified boundary.
	*   @param value Value
	*   @param boundary Boundary
	*   @return True if value is aligned */
	template<std::integral Boundary, std::integral T>
	[[nodiscard]]
	static constexpr bool is_aligned(T value) noexcept
	{
		return !(value % sizeof(Boundary));
	}

	/** Helper function to align an integer down.
	*   @param x Number
	*   @param align Alignment
	*   @return Aligned down value */
	template<std::unsigned_integral T>
	[[nodiscard]]
	static constexpr T align_down(T x, std::uint32_t align) noexcept
	{
		return x & ~(static_cast<T>(align) - 1);
	}

	/** Helper function to align an integer up.
	*   @param x Number
	*   @param align Alignment
	*   @return Aligned up value */
	template<std::unsigned_integral T>
	[[nodiscard]]
	static constexpr T align_up(T x, std::uint32_t align) noexcept
	{
		return (x & static_cast<T>(align - 1)) ? align_down(x, align) + static_cast<T>(align) : x;
	}

	template<std::unsigned_integral T>
	[[nodiscard]]
	static constexpr bool align_up_if_safe(T& x, std::uint32_t align) noexcept
	{
		if (x & static_cast<T>(align - 1))
		{
			auto result = static_cast<T>(align);
			if (add_if_safe(result, align_down(x, align)))
			{
				x = result;
				return true;
			}
			return false;
		}
		return true;
	}

	/** Checks if sum of two unsigned integers is safe (no overflow occurs).
	*   @param a Number
	*   @param b Number
	*   @return True if sum of two unsigned integers is safe (no overflow occurs) */
	template<std::unsigned_integral T>
	[[nodiscard]]
	static constexpr bool is_sum_safe(T a, T b) noexcept
	{
		return a <= (std::numeric_limits<T>::max)() - b;
	}

	/** Checks if sum of two unsigned integers is safe (no overflow occurs) and adds second one to the first, if so.
	*   @param target Number to add to
	*   @param value Value to add
	*   @return True if sum of two unsigned integers is safe (no overflow occurs) */
	template<std::unsigned_integral T>
	[[nodiscard]]
	static constexpr bool add_if_safe(T& target, T value) noexcept
	{
		return is_sum_safe(target, value) ? target += value, true : false;
	}

	[[nodiscard]]
	static constexpr bool add_offset_if_safe(std::size_t& pos, std::int32_t offset) noexcept
	{
		if (offset < 0)
		{
			if (offset == (std::numeric_limits<std::int32_t>::min)()
				|| static_cast<std::size_t>(-offset) > pos)
			{
				return false;
			}

			pos += offset;
		}
		else
		{
			if (!add_if_safe(pos, static_cast<std::size_t>(offset)))
				return false;
		}
		return true;
	}
};

} //namespace utilities
