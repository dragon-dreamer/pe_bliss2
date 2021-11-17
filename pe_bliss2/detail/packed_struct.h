#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include <boost/endian/conversion.hpp>

#include "buffers/input_buffer_interface.h"
#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/detail/packed_serialization.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/generic_error.h"

namespace pe_bliss::detail
{

template<standard_layout T, boost::endian::order Endianness = boost::endian::order::little>
class packed_struct
{
public:
	using value_type = T;

public:
	static constexpr auto endianness = Endianness;
	static constexpr auto packed_size = typename packed_serialization<Endianness>
		::template get_type_size<value_type>();

public:
	packed_struct() noexcept = default;

	explicit packed_struct(const value_type& value) noexcept
		: value_(value)
	{
	}

	packed_struct& operator=(const value_type& value) noexcept
	{
		*this = packed_struct(value);
		return *this;
	}

	void deserialize(buffers::input_buffer_interface& buf, bool allow_virtual_memory)
	{
		set_offsets(buf);
		std::array<std::byte, packed_size> data{};
		physical_size_ = buf.read(packed_size, data.data());
		if (!allow_virtual_memory && physical_size_ != packed_size)
			throw pe_error(utilities::generic_errc::buffer_overrun);

		typename packed_serialization<Endianness>::deserialize(value_, data.data());
	}

	void deserialize_until(buffers::input_buffer_interface& buf, std::size_t size, bool allow_virtual_memory)
	{
		set_offsets(buf);
		std::array<std::byte, packed_size> data{};
		size = (std::min)(size, packed_size);
		physical_size_ = buf.read(size, data.data());
		if (!allow_virtual_memory && physical_size_ != size)
			throw pe_error(utilities::generic_errc::buffer_overrun);

		typename packed_serialization<Endianness>::deserialize_until(value_, data.data(), size);
	}

	std::size_t serialize(buffers::output_buffer_interface& buf, bool write_virtual_part) const
	{
		auto data = serialize();
		auto size = write_virtual_part ? packed_size : physical_size_;
		buf.write(size, data.data());
		return size;
	}

	std::size_t serialize(std::byte* buf, std::size_t max_size, bool write_virtual_part) const
	{
		std::size_t size = write_virtual_part ? packed_size : physical_size_;
		if (size > max_size)
			throw pe_error(utilities::generic_errc::buffer_overrun);
		auto data = serialize();
		std::memcpy(buf, data.data(), size);
		return size;
	}

	std::size_t serialize_until(std::byte* buf, std::size_t size, bool write_virtual_part) const
	{
		size = (std::min)(size, write_virtual_part ? packed_size : physical_size_);
		auto data = serialize();
		std::memcpy(buf, data.data(), size);
		return size;
	}

	[[nodiscard]]
	std::array<std::byte, packed_size> serialize() const noexcept
	{
		std::array<std::byte, packed_size> data{};
		typename packed_serialization<Endianness>::serialize(value_, data.data());
		return data;
	}

	[[nodiscard]] const value_type* operator->() const noexcept
		requires (!std::is_scalar_v<value_type>)
	{
		return &value_;
	}

	[[nodiscard]] value_type* operator->() noexcept
		requires (!std::is_scalar_v<value_type>)
	{
		return &value_;
	}

	[[nodiscard]] const value_type& get() const noexcept
	{
		return value_;
	}

	[[nodiscard]] value_type& get() noexcept
	{
		return value_;
	}

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

	[[nodiscard]] bool is_virtual() const noexcept
	{
		return physical_size_ < packed_size;
	}

	void set_physical_size(std::size_t size) noexcept
	{
		physical_size_ = (std::min)(packed_size, size);
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
	void set_offsets(buffers::input_buffer_interface& buf)
	{
		buffer_pos_ = buf.rpos();
		absolute_offset_ = buffer_pos_ + buf.absolute_offset();
		relative_offset_ = buffer_pos_ + buf.relative_offset();
	}

private:
	std::size_t buffer_pos_ = 0;
	std::size_t absolute_offset_ = 0;
	std::size_t relative_offset_ = 0;
	std::size_t physical_size_ = packed_size;
	value_type value_{};
};

} //namespace pe_bliss::detail
