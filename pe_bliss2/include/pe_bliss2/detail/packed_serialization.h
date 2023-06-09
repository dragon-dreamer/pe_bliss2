#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>

#include <boost/endian/conversion.hpp>
#include <boost/pfr/core.hpp>

#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/detail/endian_convert.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "utilities/static_class.h"

namespace pe_bliss::detail
{

template<boost::endian::order StructureFieldsEndianness
	= boost::endian::order::native>
class packed_serialization : utilities::static_class
{
public:
	template<standard_layout T, byte_pointer BytePointer>
	static BytePointer deserialize(T& result, BytePointer data) noexcept
	{
		using type = std::remove_cvref_t<T>;
		if constexpr (impl::is_array<type>::value)
		{
			if constexpr (std::is_class_v<typename type::value_type>
				|| StructureFieldsEndianness != boost::endian::order::native)
			{
				for (auto& elem : result)
					data = deserialize(elem, data);
			}
			else
			{
				std::memcpy(result.data(), data, sizeof(type));
				data += sizeof(type);
			}
		}
		else if constexpr (std::is_class_v<type>)
		{
			boost::pfr::for_each_field(result, [&data](auto& value) {
				data = deserialize(value, data);
			});
		}
		else
		{
			std::memcpy(&result, data, sizeof(type));
			convert_endianness<StructureFieldsEndianness,
				boost::endian::order::native>(result);
			data += sizeof(type);
		}
		return data;
	}

	template<auto FieldPtr, standard_layout T, byte_pointer BytePointer>
	static BytePointer deserialize_until(T& result, BytePointer data) noexcept
	{
		auto stop_ptr = reinterpret_cast<std::uintptr_t>(&(result.*FieldPtr));
		boost::pfr::for_each_field(result, [&result, &data, stop_ptr] (auto& value) {
			if (reinterpret_cast<std::uintptr_t>(&value) > stop_ptr)
				return;
			data = deserialize(value, data);
		});
		return data;
	}

	template<standard_layout T, byte_pointer BytePointer>
	static BytePointer deserialize_until(T& result, BytePointer data, std::size_t size) noexcept
	{
		constexpr auto full_size = packed_reflection::get_type_size<T>();
		std::byte full[full_size]{};
		if (size > full_size)
			size = full_size;
		std::memcpy(full, data, size);
		deserialize(result, full);
		return data + size;
	}

	template<standard_layout T>
	static std::byte* serialize(const T& value, std::byte* data) noexcept
	{
		using type = std::remove_cvref_t<T>;
		if constexpr (impl::is_array<type>::value)
		{
			if constexpr (std::is_class_v<typename type::value_type>
				|| StructureFieldsEndianness != boost::endian::order::native)
			{
				for (auto& elem : value)
					data = serialize(elem, data);
			}
			else
			{
				std::memcpy(data, value.data(), sizeof(type));
				data += sizeof(type);
			}
		}
		else if constexpr (std::is_class_v<type>)
		{
			boost::pfr::for_each_field(value, [&data](const auto& value) {
				data = serialize(value, data);
			});
		}
		else
		{
			if constexpr (StructureFieldsEndianness == boost::endian::order::native)
			{
				std::memcpy(data, &value, sizeof(type));
			}
			else
			{
				struct temp { type value; } copy;
				std::memcpy(&copy.value, &value, sizeof(type));
				convert_endianness<boost::endian::order::native,
					StructureFieldsEndianness>(copy.value);
				std::memcpy(data, &copy.value, sizeof(type));
			}
			data += sizeof(type);
		}
		return data;
	}

	template<auto FieldPtr, standard_layout T>
	static std::byte* serialize_until(const T& value, std::byte* data) noexcept
	{
		auto stop_ptr = reinterpret_cast<std::uintptr_t>(&(value.*FieldPtr));
		boost::pfr::for_each_field(value, [&data, stop_ptr] (const auto& value) {
			if (reinterpret_cast<std::uintptr_t>(&value) > stop_ptr)
				return;
			data = serialize(value, data);
		});
		return data;
	}

	template<standard_layout T>
	static std::byte* serialize_until(const T& value,
		std::byte* data, std::size_t size) noexcept
	{
		constexpr auto full_size = packed_reflection::get_type_size<T>();
		std::byte full[full_size];
		serialize(value, full);
		if (size > full_size)
			size = full_size;
		std::memcpy(data, full, size);
		return data + size;
	}
};

} //namespace pe_bliss::detail
