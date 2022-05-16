#pragma once

#include <cstddef>
#include <concepts>
#include <string>
#include <utility>

#include "buffers/input_buffer_state.h"

namespace buffers
{
class input_buffer_interface;
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss
{

template<typename T>
concept convertible_to_string = std::convertible_to<T, std::string>;

class [[nodiscard]] packed_c_string
{
public:
	packed_c_string() = default;

	template<convertible_to_string String>
	packed_c_string(String&& str)
		noexcept(noexcept(std::string(std::forward<String>(str))))
		: value_(std::forward<String>(str))
	{
	}

	template<convertible_to_string String>
	packed_c_string& operator=(String&& str)
		noexcept(noexcept(value_ = std::forward<String>(str)))
	{
		value_ = std::forward<String>(str);
		state_ = {};
		virtual_nullbyte_ = false;
		return *this;
	}

	void deserialize(buffers::input_buffer_interface& buf,
		bool allow_virtual_memory);

	std::size_t serialize(buffers::output_buffer_interface& buf,
		bool write_virtual_part) const;
	std::size_t serialize(std::byte* buf, std::size_t max_size,
		bool write_virtual_part) const;

	[[nodiscard]] std::size_t physical_size() const noexcept
	{
		return value_.size() + !virtual_nullbyte_;
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

	[[nodiscard]] bool is_virtual() const noexcept
	{
		return virtual_nullbyte_;
	}

	[[nodiscard]] const std::string& value() const & noexcept
	{
		return value_;
	}

	[[nodiscard]] std::string& value() & noexcept
	{
		return value_;
	}

	[[nodiscard]] std::string value() && noexcept
	{
		return std::move(value_);
	}
	
	void set_virtual_nullbyte(bool virtual_nullbyte) noexcept
	{
		virtual_nullbyte_ = virtual_nullbyte;
	}

private:
	std::string value_;
	buffers::serialized_data_state state_;
	bool virtual_nullbyte_ = false;
};

} //namespace pe_bliss
