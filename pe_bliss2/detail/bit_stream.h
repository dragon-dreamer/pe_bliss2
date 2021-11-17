#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <concepts>
#include <climits>
#include <type_traits>
#include <system_error>

#include "utilities/generic_error.h"

namespace pe_bliss::detail
{

template<typename Container>
class bit_stream
{
public:
	static constexpr std::size_t element_bit_count
		= sizeof(typename Container::value_type) * CHAR_BIT;
	using underlying_type = std::conditional_t<
		std::is_enum_v<typename Container::value_type>,
		std::underlying_type_t<typename Container::value_type>,
		typename Container::value_type>;

public:
	explicit bit_stream(Container& container) noexcept
		: container_(container)
		, bit_count_(container.size() * CHAR_BIT)
	{
	}

	template<std::unsigned_integral T = std::uint32_t>
	[[nodiscard]]
	T read(std::size_t count)
	{
		if (count > bit_count_ - pos_ || count > sizeof(T) * CHAR_BIT)
			throw std::system_error(utilities::generic_errc::buffer_overrun);

		std::uint32_t result{};
		auto initial_count = count;
		get_remaining_value_bits(result, count, initial_count);

		for (std::size_t i = 0, len = count / element_bit_count; i != len; ++i)
		{
			result |= static_cast<std::uint32_t>(container_[pos_ / element_bit_count])
				<< (initial_count - count);
			pos_ += element_bit_count;
			count -= element_bit_count;
		}

		get_remaining_value_bits(result, count, initial_count);
		return result;
	}

	[[nodiscard]]
	std::size_t get_pos() const noexcept
	{
		return pos_;
	}

	[[nodiscard]]
	std::size_t get_bit_count() const noexcept
	{
		return bit_count_;
	}

	void set_pos(std::size_t pos) noexcept
	{
		pos_ = pos;
	}

	void set_bit_count(std::size_t bit_count) noexcept
	{
		bit_count_ = bit_count;
	}

private:
	void get_remaining_value_bits(std::uint32_t& result,
		std::size_t& count, std::size_t initial_count)
		noexcept(noexcept(container_[0]))
	{
		if (!count)
			return;

		std::size_t used_bits = pos_ % element_bit_count;
		std::size_t remaining_bits = element_bit_count - used_bits;
		if (!remaining_bits)
			return;

		remaining_bits = (std::min)(remaining_bits, count);

		auto bit_mask = (1u << remaining_bits) - 1u;
		bit_mask <<= used_bits;

		auto byte = static_cast<underlying_type>(container_[pos_ / element_bit_count]);
		result |= ((byte & bit_mask) >> used_bits) << (initial_count - count);

		count -= remaining_bits;
		pos_ += remaining_bits;
	}

private:
	Container& container_;
	std::size_t bit_count_;
	std::size_t pos_ = 0;
};

} //namespace pe_bliss::detail
