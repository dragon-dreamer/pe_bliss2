#pragma once

#include <cstddef>
#include <compare>
#include <concepts>
#include <string>
#include <utility>

#include "buffers/input_buffer_state.h"

namespace buffers
{
class input_buffer_stateful_wrapper_ref;
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

	template<typename... Args>
		requires(std::constructible_from<std::string, Args...>)
	explicit packed_c_string(Args&&... args)
		noexcept(noexcept(std::string(std::forward<Args>(args)...)))
		: value_(std::forward<Args>(args)...)
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

	void deserialize(buffers::input_buffer_stateful_wrapper_ref& buf,
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

	[[nodiscard]]
	friend auto operator<=>(const packed_c_string& l, const packed_c_string& r) noexcept
	{
		return l.value() <=> r.value();
	}

	[[nodiscard]]
	friend bool operator==(const packed_c_string& l, const packed_c_string& r) noexcept
	{
		return l.value() == r.value();
	}

private:
	std::string value_;
	buffers::serialized_data_state state_;
	bool virtual_nullbyte_ = false;
};

} //namespace pe_bliss
