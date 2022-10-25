#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

#include "buffers/input_buffer_state.h"

namespace buffers
{
class input_buffer_stateful_wrapper_ref;
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::detail
{

class packed_byte_array_base
{
public:
	[[nodiscard]] constexpr std::size_t physical_size() const noexcept
	{
		return physical_size_;
	}

	[[nodiscard]] constexpr std::size_t data_size() const noexcept
	{
		return data_size_;
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

	[[nodiscard]] constexpr bool is_virtual() const noexcept
	{
		return data_size_ != physical_size_;
	}

protected:
	void deserialize_impl(buffers::input_buffer_stateful_wrapper_ref& buf,
		std::byte* data, std::size_t size, std::size_t max_size,
		bool allow_virtual_data);
	std::size_t serialize_impl(buffers::output_buffer_interface& buf,
		const std::byte* data, bool write_virtual_part) const;
	std::size_t serialize_impl(std::byte* buf, std::size_t max_size,
		const std::byte* data, bool write_virtual_part) const;

protected:
	std::size_t data_size_ = 0;
	std::size_t physical_size_ = 0;
	buffers::serialized_data_state state_;
};

} //namespace pe_bliss::detail

namespace pe_bliss
{

template<std::size_t MaxSize>
class [[nodiscard]] packed_byte_array : public detail::packed_byte_array_base
{
public:
	static constexpr auto max_size = MaxSize;

public:
	using array_type = std::array<std::byte, MaxSize>;

public:
	void deserialize(buffers::input_buffer_stateful_wrapper_ref& buf,
		std::size_t size, bool allow_virtual_data)
	{
		value_ = {};
		deserialize_impl(buf, value_.data(), size, max_size, allow_virtual_data);
	}

	std::size_t serialize(buffers::output_buffer_interface& buf,
		bool write_virtual_part) const
	{
		return serialize_impl(buf, value_.data(), write_virtual_part);
	}

	std::size_t serialize(std::byte* buf,
		std::size_t max_size, bool write_virtual_part) const
	{
		return serialize_impl(buf, max_size, value_.data(), write_virtual_part);
	}

	[[nodiscard]] constexpr const array_type& value() const noexcept
	{
		return value_;
	}

	[[nodiscard]] constexpr array_type& value() noexcept
	{
		return value_;
	}

	[[nodiscard]] constexpr std::byte operator[](std::size_t index) const noexcept
	{
		return value_[index];
	}

	[[nodiscard]] constexpr std::byte& operator[](std::size_t index) noexcept
	{
		return value_[index];
	}

	constexpr void set_physical_size(std::size_t size) noexcept
	{
		physical_size_ = (std::min)(max_size, size);
		if (data_size_ < physical_size_)
			data_size_ = physical_size_;
	}

	constexpr void set_data_size(std::size_t size) noexcept
	{
		data_size_ = (std::min)(max_size, size);
		if (data_size_ < physical_size_)
			data_size_ = physical_size_;
	}

	template<typename Other>
	constexpr void copy_metadata_from(const Other& other)
		noexcept(noexcept(other.get_state())
			&& noexcept(other.physical_size())
			&& noexcept(other.data_size()))
	{
		state_ = other.get_state();
		set_physical_size(other.physical_size());
		set_data_size(other.data_size());
	}

private:
	array_type value_{};
};

} //namespace pe_bliss
