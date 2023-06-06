#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include <boost/pfr/core.hpp>

#include "pe_bliss2/detail/concepts.h"
#include "utilities/static_class.h"

namespace pe_bliss::detail
{

class packed_reflection : utilities::static_class
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
			return (0u + ... + get_type_size<std::remove_cvref_t<decltype(args)>>());
		},
		boost::pfr::structure_tie(val));
	}

	template<standard_layout T>
	static constexpr bool get_field_offset_impl(
		const T& field, const void* field_ptr,
		std::size_t& offset, std::size_t& result) noexcept
	{
		if (static_cast<const void*>(&field) == field_ptr)
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
		auto field_ptr = &(val.*FieldPtr);
		std::apply([&offset, &result, field_ptr] (auto&&... args) {
			(... && get_field_offset_impl<std::remove_cvref_t<decltype(args)>>(
				args, field_ptr, offset, result));
		},
		boost::pfr::structure_tie(val));
		return result;
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
};

} //namespace pe_bliss::detail
