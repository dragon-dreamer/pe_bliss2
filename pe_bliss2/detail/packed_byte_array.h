#pragma once

#include <array>
#include <cassert>
#include <cstddef>

namespace buffers
{
class input_buffer_interface;
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::detail
{

class packed_byte_array_base
{
public:
	[[nodiscard]] std::size_t physical_size() const noexcept
	{
		return physical_size_;
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

	[[nodiscard]] std::size_t data_size() const noexcept
	{
		return data_size_;
	}

	[[nodiscard]] bool is_virtual() const noexcept
	{
		return data_size_ != physical_size_;
	}

protected:
	void deserialize_impl(buffers::input_buffer_interface& buf, std::byte* data,
		std::size_t size, bool allow_virtual_memory);
	std::size_t serialize_impl(buffers::output_buffer_interface& buf, const std::byte* data,
		bool write_virtual_part) const;
	std::size_t serialize_impl(std::byte* buf, std::size_t max_size,
		const std::byte* data, bool write_virtual_part) const;

protected:
	std::size_t data_size_ = 0;
	std::size_t physical_size_ = 0;
	std::size_t buffer_pos_ = 0;
	std::size_t absolute_offset_ = 0;
	std::size_t relative_offset_ = 0;
};

template<std::size_t MaxSize>
class packed_byte_array : public packed_byte_array_base
{
public:
	static constexpr auto max_size = MaxSize;

public:
	using array_type = std::array<std::byte, MaxSize>;

public:
	void deserialize(buffers::input_buffer_interface& buf, std::size_t size, bool allow_virtual_memory)
	{
		assert(size <= MaxSize);
		value_ = {};
		deserialize_impl(buf, value_.data(), size, allow_virtual_memory);
	}

	std::size_t serialize(buffers::output_buffer_interface& buf, bool write_virtual_part) const
	{
		return serialize_impl(buf, value_.data(), write_virtual_part);
	}

	std::size_t serialize(std::byte* buf,
		std::size_t max_size, bool write_virtual_part) const
	{
		return serialize_impl(buf, max_size, value_.data(), write_virtual_part);
	}

	[[nodiscard]] const array_type& value() const noexcept
	{
		return value_;
	}

	[[nodiscard]] array_type& value() noexcept
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

	void set_physical_size(std::size_t size) noexcept
	{
		physical_size_ = (std::min)(max_size, size);
	}

	void set_relative_offset(std::size_t offset) noexcept
	{
		relative_offset_ = offset;
	}

	void set_absolute_offset(std::size_t offset) noexcept
	{
		absolute_offset_ = offset;
	}

	void set_buffer_pos(std::size_t pos) noexcept
	{
		buffer_pos_ = pos;
	}

	template<typename Other>
	void copy_metadata_from(const Other& other) noexcept
	{
		buffer_pos_ = other.buffer_pos();
		absolute_offset_ = other.absolute_offset();
		relative_offset_ = other.relative_offset();
		set_physical_size(other.physical_size());
	}

private:
	array_type value_{};
};

} //namespace pe_bliss::detail
