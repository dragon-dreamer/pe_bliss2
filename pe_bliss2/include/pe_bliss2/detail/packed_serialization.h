#pragma once

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

namespace pe_bliss::detail
{

template<boost::endian::order StructureFieldsEndianness
	= boost::endian::order::native>
class packed_serialization
{
private:
	template<typename Value, byte_pointer BytePointer>
	static byte_pointer auto deserialize_value(Value& value,
		BytePointer data) noexcept
	{
		using type = std::remove_cvref_t<Value>;
		if constexpr (std::is_class_v<type>)
		{
			data = deserialize(value, data);
		}
		else if constexpr (std::is_array_v<type>
			&& std::is_class_v<std::remove_all_extents_t<type>>)
		{
			for (auto& elem : value)
				data = deserialize(elem, data);
		}
		else
		{
			std::memcpy(&value, data, sizeof(type));
			convert_endianness<StructureFieldsEndianness,
				boost::endian::order::native>(value);
			data += sizeof(type);
		}
		return data;
	}

	template<typename Value, byte_pointer BytePointer>
	static byte_pointer auto serialize_value(const Value& value, BytePointer data) noexcept
	{
		using type = std::remove_cvref_t<decltype(value)>;
		if constexpr (std::is_class_v<type>)
		{
			data = serialize(value, data);
		}
		else if constexpr (std::is_array_v<type>
			&& std::is_class_v<std::remove_all_extents_t<type>>)
		{
			for (auto& elem : value)
				data = serialize(elem, data);
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

public:
	template<standard_layout T, byte_pointer BytePointer>
	static BytePointer deserialize(T& result, BytePointer data) noexcept
	{
		boost::pfr::for_each_field(result, [&data] (auto& value) {
			data = deserialize_value(value, data);
		});
		return data;
	}

	template<auto FieldPtr, standard_layout T, byte_pointer BytePointer>
	static BytePointer deserialize_until(T& result, BytePointer data) noexcept
	{
		auto stop_ptr = reinterpret_cast<std::uintptr_t>(&(result.*FieldPtr));
		boost::pfr::for_each_field(result, [&result, &data, stop_ptr] (auto& value) {
			if (reinterpret_cast<std::uintptr_t>(&value) > stop_ptr)
				return;
			data = deserialize_value(value, data);
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
		boost::pfr::for_each_field(value, [&data] (const auto& value) {
			data = serialize_value(value, data);
		});
		return data;
	}

	template<auto FieldPtr, standard_layout T>
	static std::byte* serialize_until(const T& value, std::byte* data) noexcept
	{
		auto stop_ptr = reinterpret_cast<std::uintptr_t>(&(value.*FieldPtr));
		boost::pfr::for_each_field(value, [&data, stop_ptr] (const auto& value) {
			if (reinterpret_cast<std::uintptr_t>(&value) > stop_ptr)
				return;
			data = serialize_value(value, data);
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
