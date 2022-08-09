#pragma once

#include <cstddef>
#include <compare>

namespace buffers
{

class input_buffer_stateful_wrapper_ref;

class [[nodiscard]] input_buffer_state
{
public:
	constexpr input_buffer_state() noexcept = default;
	explicit input_buffer_state(const input_buffer_stateful_wrapper_ref& wrapper);

	[[nodiscard]]
	constexpr std::size_t buffer_pos() const noexcept
	{
		return buffer_pos_;
	}

	[[nodiscard]]
	constexpr std::size_t absolute_offset() const noexcept
	{
		return absolute_offset_;
	}

	[[nodiscard]]
	constexpr std::size_t relative_offset() const noexcept
	{
		return relative_offset_;
	}

	constexpr void set_relative_offset(std::size_t offset) noexcept
	{
		relative_offset_ = offset;
	}

	constexpr void set_absolute_offset(std::size_t offset) noexcept
	{
		absolute_offset_ = offset;
	}

	constexpr void set_buffer_pos(std::size_t pos) noexcept
	{
		buffer_pos_ = pos;
	}

	friend constexpr auto operator<=>(const input_buffer_state&,
		const input_buffer_state&) noexcept = default;
	
protected:
	std::size_t buffer_pos_ = 0;
	std::size_t absolute_offset_ = 0;
	std::size_t relative_offset_ = 0;
};

class [[nodiscard]] serialized_data_state : public input_buffer_state
{
public:
	serialized_data_state() = default;
	explicit serialized_data_state(const input_buffer_stateful_wrapper_ref& wrapper);
};

} //namespace buffers
