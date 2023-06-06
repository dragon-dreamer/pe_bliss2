#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include <boost/endian/conversion.hpp>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_buffer_state.h"
#include "buffers/output_buffer_interface.h"
#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/detail/packed_serialization.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/generic_error.h"

namespace pe_bliss
{

template<detail::standard_layout T, boost::endian::order Endianness
	= boost::endian::order::little>
class [[nodiscard]] packed_struct
{
public:
	using value_type = T;

public:
	static constexpr auto endianness = Endianness;
	static constexpr auto packed_size = detail::packed_reflection
		::get_type_size<value_type>();

public:
	constexpr packed_struct() noexcept = default;

	constexpr packed_struct(const value_type& value) noexcept
		: value_(value)
	{
	}

	constexpr packed_struct& operator=(const value_type& value) noexcept
	{
		*this = packed_struct(value);
		return *this;
	}

	void deserialize(buffers::input_buffer_stateful_wrapper_ref& buf, bool allow_virtual_data)
	{
		buffers::serialized_data_state state(buf);
		std::array<std::byte, packed_size> data{};
		auto physical_size = buf.read(packed_size, data.data());
		if (!allow_virtual_data && physical_size != packed_size)
			throw pe_error(utilities::generic_errc::buffer_overrun);

		typename detail::packed_serialization<Endianness>
			::deserialize(value_, data.data());

		physical_size_ = physical_size;
		state_ = state;
	}

	void deserialize_until(buffers::input_buffer_stateful_wrapper_ref& buf,
		std::size_t size, bool allow_virtual_data)
	{
		buffers::serialized_data_state state(buf);
		std::array<std::byte, packed_size> data{};
		size = (std::min)(size, packed_size);
		auto physical_size = buf.read(size, data.data());
		if (!allow_virtual_data && physical_size != size)
			throw pe_error(utilities::generic_errc::buffer_overrun);

		typename detail::packed_serialization<Endianness>
			::deserialize_until(value_, data.data(), size);

		physical_size_ = physical_size;
		state_ = state;
	}

	std::size_t serialize(buffers::output_buffer_interface& buf,
		bool write_virtual_part) const
	{
		auto data = serialize();
		auto size = write_virtual_part ? packed_size : physical_size_;
		buf.write(size, data.data());
		return size;
	}

	std::size_t serialize(std::byte* buf, std::size_t max_size,
		bool write_virtual_part) const
	{
		std::size_t size = write_virtual_part ? packed_size : physical_size_;
		if (size > max_size)
			throw pe_error(utilities::generic_errc::buffer_overrun);
		auto data = serialize();
		std::memcpy(buf, data.data(), size);
		return size;
	}

	std::size_t serialize_until(std::byte* buf, std::size_t size,
		bool write_virtual_part) const
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
		typename detail::packed_serialization<Endianness>
			::serialize(value_, data.data());
		return data;
	}

	[[nodiscard]] constexpr const value_type* operator->() const noexcept
		requires (!std::is_scalar_v<value_type>)
	{
		return &value_;
	}

	[[nodiscard]] constexpr value_type* operator->() noexcept
		requires (!std::is_scalar_v<value_type>)
	{
		return &value_;
	}

	[[nodiscard]] constexpr const value_type& get() const noexcept
	{
		return value_;
	}

	[[nodiscard]] constexpr value_type& get() noexcept
	{
		return value_;
	}

	[[nodiscard]] constexpr std::size_t physical_size() const noexcept
	{
		return physical_size_;
	}

	[[nodiscard]] constexpr bool is_virtual() const noexcept
	{
		return physical_size_ < packed_size;
	}

	constexpr void set_physical_size(std::size_t size) noexcept
	{
		physical_size_ = (std::min)(packed_size, size);
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

	template<typename Other>
	constexpr void copy_metadata_from(const Other& other)
		noexcept(noexcept(other.get_state())
			&& noexcept(other.physical_size()))
	{
		state_ = other.get_state();
		set_physical_size(other.physical_size());
	}

	[[nodiscard]]
	constexpr std::size_t data_size() const noexcept
	{
		return packed_size;
	}

private:
	buffers::serialized_data_state state_;
	std::size_t physical_size_ = packed_size;
	value_type value_{};
};

} //namespace pe_bliss
