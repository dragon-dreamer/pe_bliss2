#pragma once

#include <vector>
#include <cassert>
#include <cstddef>

namespace buffers
{
class input_buffer_interface;
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::detail
{

class packed_byte_vector
{
public:
	using vector_type = std::vector<std::byte>;

public:
	void deserialize(buffers::input_buffer_interface& buf, std::size_t size, bool allow_virtual_memory);
	std::size_t serialize(buffers::output_buffer_interface& buf, bool write_virtual_part) const;
	std::size_t serialize(std::byte* buf,
		std::size_t max_size, bool write_virtual_part) const;

	[[nodiscard]] const vector_type& value() const noexcept
	{
		return value_;
	}

	[[nodiscard]] vector_type& value() noexcept
	{
		return value_;
	}

	[[nodiscard]] std::byte operator[](std::size_t index) const noexcept
	{
		return value_[index];
	}

	[[nodiscard]] std::byte& operator[](std::size_t index) noexcept
	{
		return value_[index];
	}

	[[nodiscard]] std::size_t data_size() const noexcept
	{
		return virtual_size_;
	}

	[[nodiscard]] std::size_t physical_size() const noexcept
	{
		return value_.size();
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
		return value_.size() != virtual_size_;
	}

private:
	vector_type value_;
	std::size_t virtual_size_ = 0;
	std::size_t buffer_pos_ = 0;
	std::size_t absolute_offset_ = 0;
	std::size_t relative_offset_ = 0;
};

} //namespace pe_bliss::detail
