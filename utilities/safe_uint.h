#pragma once

#include <compare>
#include <concepts>
#include <cstdint>
#include <system_error>
#include <type_traits>

#include "utilities/generic_error.h"
#include "utilities/math.h"

namespace utilities
{

template<std::unsigned_integral T> requires (!std::is_same_v<T, bool>)
class safe_uint
{
public:
	using value_type = T;

public:
	safe_uint() noexcept = default;
	safe_uint(const safe_uint&) noexcept = default;
	safe_uint(safe_uint&&) noexcept = default;
	safe_uint(value_type value) noexcept
		: value_(value)
	{
	}

	safe_uint& operator=(const safe_uint&) noexcept = default;
	safe_uint& operator=(safe_uint&&) noexcept = default;
	safe_uint& operator=(value_type value) noexcept
	{
		value_ = value;
		return *this;
	}

	[[nodiscard]]
	auto operator<=>(const safe_uint&) const noexcept = default;

	[[nodiscard]]
	explicit operator bool() const noexcept
	{
		return value_ != value_type{};
	}

	template<std::unsigned_integral Other>
	[[nodiscard]]
	friend safe_uint operator+(safe_uint value, Other other)
	{
		if (static_cast<T>(other) != other)
			throw std::system_error(generic_errc::integer_overflow);

		if (!math::add_if_safe(value.value_, static_cast<T>(other)))
			throw std::system_error(generic_errc::integer_overflow);

		return value;
	}

	template<std::unsigned_integral Other>
	[[nodiscard]]
	friend safe_uint operator+(safe_uint value, safe_uint<Other> other)
	{
		return value + other.value();
	}

	template<std::unsigned_integral Other>
	[[nodiscard]]
	friend safe_uint operator-(safe_uint value, Other other)
	{
		if (other > value.value_)
			throw std::system_error(generic_errc::integer_overflow);

		value.value_ -= static_cast<value_type>(other);
		return value;
	}

	template<std::unsigned_integral Other>
	[[nodiscard]]
	friend safe_uint operator-(safe_uint value, safe_uint<Other> other)
	{
		return value - other.value();
	}

	template<std::unsigned_integral Other>
	safe_uint& operator+=(Other other)
	{
		*this = *this + other;
		return *this;
	}

	template<std::unsigned_integral Other>
	safe_uint& operator-=(Other other)
	{
		*this = *this - other;
		return *this;
	}

	template<std::unsigned_integral Other>
	safe_uint& operator+=(safe_uint<Other> other)
	{
		*this = *this + other;
		return *this;
	}

	template<std::unsigned_integral Other>
	safe_uint& operator-=(safe_uint<Other> other)
	{
		*this = *this - other;
		return *this;
	}

	void align_up(std::uint32_t alignment)
	{
		if (!math::align_up_if_safe(value_, alignment))
			throw std::system_error(generic_errc::integer_overflow);
	}

	[[nodiscard]]
	value_type value() const noexcept
	{
		return value_;
	}

private:
	value_type value_{};
};

} //namespace utilities
