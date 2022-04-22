#pragma once

#include <cstddef>
#include <cstdint>
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

class [[nodiscard]] packed_utf16_string
{
public:
	using string_type = std::u16string;

public:
	void deserialize(buffers::input_buffer_interface& buf,
		bool allow_virtual_memory);
	std::size_t serialize(buffers::output_buffer_interface& buf,
		bool write_virtual_part) const;
	std::size_t serialize(std::byte* buf,
		std::size_t max_size, bool write_virtual_part) const;

	[[nodiscard]] constexpr const string_type& value() const & noexcept
	{
		return value_;
	}

	[[nodiscard]] constexpr string_type& value() & noexcept
	{
		return value_;
	}

	[[nodiscard]] constexpr string_type&& value() && noexcept
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
		return virtual_size_;
	}

	[[nodiscard]] constexpr std::size_t physical_size() const noexcept
	{
		return physical_size_;
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
		return physical_size_ < virtual_size_;
	}

	[[nodiscard]] std::size_t virtual_string_length() const noexcept;

	void set_data_size(std::size_t size) noexcept;

	void set_physical_size(std::size_t size) noexcept;

	void sync_physical_size() noexcept;

private:
	template<typename WriteChar, typename WriteRemaining>
	std::size_t serialize(WriteChar&& write_part,
		WriteRemaining&& write_remaining,
		bool write_virtual_part) const;

private:
	string_type value_;
	std::size_t virtual_size_ = 0;
	std::size_t physical_size_ = 0;
	buffers::serialized_data_state state_;
};

} //namespace pe_bliss
