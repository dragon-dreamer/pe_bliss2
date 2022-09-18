#pragma once

#include <cstddef>
#include <compare>
#include <concepts>
#include <limits>
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

template<typename String>
class [[nodiscard]] packed_c_string_base
{
public:
	using string_type = String;

public:
	packed_c_string_base() = default;

	template<typename... Args>
		requires(std::constructible_from<string_type, Args...>)
	explicit packed_c_string_base(Args&&... args)
		noexcept(noexcept(string_type(std::forward<Args>(args)...)))
		: value_(std::forward<Args>(args)...)
	{
	}

	template<std::convertible_to<string_type> String>
	packed_c_string_base& operator=(String&& str)
		noexcept(noexcept(value_ = std::forward<String>(str)))
	{
		value_ = std::forward<String>(str);
		state_ = {};
		virtual_nullbyte_ = false;
		return *this;
	}

	void deserialize(buffers::input_buffer_stateful_wrapper_ref& buf,
		bool allow_virtual_memory,
		std::size_t max_physical_size = (std::numeric_limits<std::size_t>::max)());

	std::size_t serialize(buffers::output_buffer_interface& buf,
		bool write_virtual_part) const;
	std::size_t serialize(std::byte* buf, std::size_t max_size,
		bool write_virtual_part) const;

	[[nodiscard]] std::size_t physical_size() const noexcept
	{
		return (value_.size() + !virtual_nullbyte_)
			* sizeof(typename string_type::value_type);
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

	[[nodiscard]] const string_type& value() const & noexcept
	{
		return value_;
	}

	[[nodiscard]] string_type& value() & noexcept
	{
		return value_;
	}

	[[nodiscard]] string_type value() && noexcept
	{
		return std::move(value_);
	}
	
	void set_virtual_nullbyte(bool virtual_nullbyte) noexcept
	{
		virtual_nullbyte_ = virtual_nullbyte;
	}

	[[nodiscard]]
	friend auto operator<=>(const packed_c_string_base& l, const packed_c_string_base& r) noexcept
	{
		return l.value() <=> r.value();
	}

	[[nodiscard]]
	friend bool operator==(const packed_c_string_base& l, const packed_c_string_base& r) noexcept
	{
		return l.value() == r.value();
	}

private:
	string_type value_;
	buffers::serialized_data_state state_;
	bool virtual_nullbyte_ = false;
};

using packed_c_string = packed_c_string_base<std::string>;
using packed_utf16_c_string = packed_c_string_base<std::string>;

} //namespace pe_bliss
