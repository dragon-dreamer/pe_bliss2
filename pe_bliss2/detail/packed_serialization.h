#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <tuple>
#include <type_traits>
#include <utility>

#include <boost/endian/conversion.hpp>
#include <boost/pfr/core.hpp>

#include "pe_bliss2/detail/concepts.h"

namespace pe_bliss::detail
{

template<boost::endian::order StructureFieldsEndianness = boost::endian::order::native>
class packed_serialization
{
private:
	template<typename T> struct member_type_helper;
	template<typename R, standard_layout T>
	struct member_type_helper<R (T::*)>
	{
		using member_type = R;
		using class_type = T;
	};

	template<standard_layout T>
	static consteval auto get_fields_size()
	{
		T val{};
		return std::apply([] (auto&&... args) {
			return (... + get_type_size<std::remove_cvref_t<decltype(args)>>());
		},
		boost::pfr::structure_tie(val));
	}

	template<boost::endian::order From, boost::endian::order To, typename Val>
	static void convert_endianness(Val& value) noexcept
	{
		if constexpr (From != To)
		{
			if constexpr (std::is_array_v<std::remove_cvref_t<Val>>)
			{
				for (auto& elem : value)
					convert_endianness<From, To>(elem);
			}
			else
			{
				boost::endian::conditional_reverse_inplace<From, To>(value);
			}
		}
	}

	template<standard_layout T>
	static constexpr bool get_field_offset_impl(
		const T& field, const void* fieldPtr, std::size_t& offset, std::size_t& result) noexcept
	{
		if (static_cast<const void*>(&field) == fieldPtr)
		{
			result = offset;
			return false;
		}

		offset += get_type_size<T>();
		return true;
	}

	template<auto FieldPtr>
	static consteval std::size_t get_field_offset_impl() noexcept
	{
		using class_type = typename member_type_helper<decltype(FieldPtr)>::class_type;
		std::size_t result = sizeof(class_type);
		std::size_t offset = 0;
		class_type val{};
		auto fieldPtr = &(val.*FieldPtr);
		std::apply([&offset, &result, fieldPtr] (auto&&... args) {
			(... && get_field_offset_impl<std::remove_cvref_t<decltype(args)>>(args, fieldPtr, offset, result));
		},
		boost::pfr::structure_tie(val));
		return result;
	}

	template<typename Value, byte_pointer BytePointer>
	static byte_pointer auto deserialize_value(Value& value, BytePointer data) noexcept
	{
		using type = std::remove_cvref_t<Value>;
		if constexpr (std::is_class_v<type>)
		{
			data = deserialize(value, data);
		}
		else if constexpr (std::is_array_v<type> && std::is_class_v<std::remove_all_extents_t<type>>)
		{
			for (auto& elem : value)
				data = deserialize(elem, data);
		}
		else
		{
			std::memcpy(&value, data, sizeof(type));
			convert_endianness<StructureFieldsEndianness, boost::endian::order::native>(value);
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
		else if constexpr (std::is_array_v<type> && std::is_class_v<std::remove_all_extents_t<type>>)
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
				convert_endianness<boost::endian::order::native, StructureFieldsEndianness>(copy.value);
				std::memcpy(data, &copy.value, sizeof(type));
			}
			data += sizeof(type);
		}
		return data;
	}

public:
	template<standard_layout T>
	[[nodiscard]] static consteval auto get_type_size()
	{
		using type = std::remove_cvref_t<T>;
		if constexpr (std::is_class_v<type>)
		{
			return get_fields_size<type>();
		}
		else if constexpr (std::is_array_v<type>)
		{
			using base_type = std::remove_all_extents_t<type>;
			return get_type_size<base_type>() * sizeof(type) / sizeof(base_type);
		}
		else
		{
			return sizeof(type);
		}
	}

	template<auto FieldPtr>
	[[nodiscard]] static consteval std::size_t get_field_offset()
	{
		constexpr auto result = get_field_offset_impl<FieldPtr>();
		using class_type = typename member_type_helper<decltype(FieldPtr)>::class_type;
		static_assert(result < get_type_size<class_type>(),
			"Specified field was not found in provided structure");
		return result;
	}

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
	static BytePointer deserialize_until(T& result, BytePointer data, std::size_t bytes) noexcept
	{
		auto stop_ptr = data + bytes;
		boost::pfr::for_each_field(result, [&result, &data, stop_ptr] (auto& value) {
			if (data + get_type_size<std::remove_cvref_t<decltype(value)>>() > stop_ptr)
				return;
			data = deserialize_value(value, data);
		});
		return data;
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
	static std::byte* serialize_until(const T& value, std::byte* data, std::size_t size) noexcept
	{
		auto stop_ptr = data + size;
		boost::pfr::for_each_field(value, [&data, stop_ptr] (const auto& value) {
			if (data + get_type_size<std::remove_cvref_t<decltype(value)>>() > stop_ptr)
				return;
			data = serialize_value(value, data);
		});
		return data;
	}
};

} //namespace pe_bliss::detail
