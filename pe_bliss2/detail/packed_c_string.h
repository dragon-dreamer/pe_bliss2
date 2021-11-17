#pragma once

#include <cstddef>
#include <string>
#include <utility>

namespace buffers
{
class input_buffer_interface;
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::detail
{

class packed_c_string
{
public:
	packed_c_string() = default;

	template<typename String>
	packed_c_string(String&& str)
		: value_(std::forward<String>(str))
	{
	}

	template<typename String>
	packed_c_string& operator=(String&& str)
	{
		value_ = std::forward<String>(str);
		buffer_pos_ = 0;
		virtual_nullbyte_ = false;
		return *this;
	}

	void deserialize(buffers::input_buffer_interface& buf, bool allow_virtual_memory);

	std::size_t serialize(buffers::output_buffer_interface& buf, bool write_virtual_part) const;
	std::size_t serialize(std::byte* buf, std::size_t max_size, bool write_virtual_part) const;

	[[nodiscard]] std::size_t physical_size() const noexcept
	{
		return value_.size() + !virtual_nullbyte_;
	}

	[[nodiscard]] std::size_t buffer_pos() const noexcept
	{
		return buffer_pos_;
	}

	[[nodiscard]] std::size_t absolute_offset() const noexcept
	{
		return absolute_offset_;
	}

	[[nodiscard]] std::size_t relative_offset() const noexcept
	{
		return relative_offset_;
	}

	[[nodiscard]] bool is_virtual() const noexcept
	{
		return virtual_nullbyte_;
	}

	[[nodiscard]] const std::string& value() const noexcept
	{
		return value_;
	}

	[[nodiscard]] std::string& value() noexcept
	{
		return value_;
	}

private:
	std::string value_;
	std::size_t buffer_pos_ = 0;
	std::size_t absolute_offset_ = 0;
	std::size_t relative_offset_ = 0;
	bool virtual_nullbyte_ = false;
};

} //namespace pe_bliss::detail
