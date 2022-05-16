#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>
#include <utility>

#include "buffers/input_buffer_state.h"

namespace buffers
{
class input_buffer_interface;
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss
{

class [[nodiscard]] packed_byte_vector
{
public:
	using vector_type = std::vector<std::byte>;

public:
	void deserialize(buffers::input_buffer_interface& buf,
		std::size_t size, bool allow_virtual_memory);
	std::size_t serialize(buffers::output_buffer_interface& buf,
		bool write_virtual_part) const;
	std::size_t serialize(std::byte* buf,
		std::size_t max_size, bool write_virtual_part) const;

	[[nodiscard]] constexpr const vector_type& value() const & noexcept
	{
		return value_;
	}

	[[nodiscard]] constexpr vector_type& value() & noexcept
	{
		return value_;
	}

	[[nodiscard]] constexpr vector_type value() && noexcept
	{
		return std::move(value_);
	}

	[[nodiscard]] constexpr decltype(auto) operator[](
		std::size_t index) const noexcept
	{
		return value_[index];
	}

	[[nodiscard]] constexpr decltype(auto) operator[](
		std::size_t index) noexcept
	{
		return value_[index];
	}

	[[nodiscard]] constexpr std::size_t data_size() const noexcept
	{
		return (std::max)(virtual_size_, physical_size());
	}

	[[nodiscard]] constexpr std::size_t physical_size() const noexcept
	{
		return value_.size();
	}

	[[nodiscard]] constexpr bool is_virtual() const noexcept
	{
		return value_.size() < virtual_size_;
	}

	[[nodiscard]]
	constexpr buffers::serialized_data_state& get_state() noexcept
	{
		return state_;
	}

	[[nodiscard]]
	constexpr const buffers::serialized_data_state& get_state() const noexcept
	{
		return state_;
	}

	constexpr void set_data_size(std::size_t size) noexcept
	{
		virtual_size_ = size;
	}

private:
	vector_type value_;
	std::size_t virtual_size_ = 0;
	buffers::serialized_data_state state_;
};

} //namespace pe_bliss
