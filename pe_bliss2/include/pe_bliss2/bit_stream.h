#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <concepts>
#include <climits>
#include <type_traits>
#include <system_error>

#include "utilities/generic_error.h"

namespace pe_bliss
{

template<typename Container>
class [[nodiscard]] bit_stream
{
public:
	using underlying_type = typename std::conditional_t<
		std::is_enum_v<typename Container::value_type>,
		std::underlying_type<typename Container::value_type>,
		std::type_identity<typename Container::value_type>>::type;
	static_assert(std::is_unsigned_v<underlying_type>);
	static constexpr std::size_t element_bit_count
		= sizeof(underlying_type) * CHAR_BIT;

public:
	explicit bit_stream(Container& container)
		noexcept(noexcept(container.size()))
		: container_(container)
		, bit_count_(container.size() * element_bit_count)
	{
	}

	template<std::unsigned_integral T = std::uint32_t>
	[[nodiscard]]
	T read(std::size_t count)
	{
		using longer_type = std::conditional_t<
			(sizeof(underlying_type) > sizeof(T)),
			underlying_type, T>;

		if (count > bit_count_ - pos_ || count > sizeof(T) * CHAR_BIT)
			throw std::system_error(utilities::generic_errc::buffer_overrun);

		T result{};
		auto initial_count = count;
		get_remaining_value_bits<longer_type>(result, count, initial_count);

		for (std::size_t i = 0, len = count / element_bit_count; i != len; ++i)
		{
			result |= static_cast<longer_type>(container_[pos_ / element_bit_count])
				<< (initial_count - count);
			pos_ += element_bit_count;
			count -= element_bit_count;
		}

		get_remaining_value_bits<longer_type>(result, count, initial_count);
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

	void set_pos(std::size_t pos)
	{
		if (pos > bit_count_)
			throw std::system_error(utilities::generic_errc::buffer_overrun);
		pos_ = pos;
	}

private:
	template<typename LongerType, typename T>
	void get_remaining_value_bits(T& result,
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

		static constexpr auto max_bits = static_cast<std::size_t>(
			std::numeric_limits<LongerType>::digits);
		auto bit_mask = remaining_bits == max_bits
			? LongerType(-1)
			: (LongerType(1u) << remaining_bits) - LongerType(1u);
		bit_mask <<= used_bits;

		auto byte = static_cast<LongerType>(container_[pos_ / element_bit_count]);
		result |= ((byte & bit_mask) >> used_bits) << (initial_count - count);

		count -= remaining_bits;
		pos_ += remaining_bits;
	}

private:
	Container& container_;
	std::size_t bit_count_;
	std::size_t pos_ = 0;
};

} //namespace pe_bliss
