#pragma once

#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/detail/imports/image_import_descriptor.h"
#include "pe_bliss2/packed_c_string.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/pe_types.h"

namespace pe_bliss::imports
{

template<detail::executable_pointer Va>
class [[nodiscard]] imported_function_address
{
public:
	using optional_va_type = std::optional<packed_struct<Va>>;

public:
	[[nodiscard]]
	optional_va_type& get_imported_va() noexcept
	{
		return imported_va_;
	}
	
	[[nodiscard]]
	const optional_va_type& get_imported_va() const noexcept
	{
		return imported_va_;
	}

private:
	optional_va_type imported_va_;
};

using ordinal_type = std::uint16_t;

template<detail::executable_pointer Va>
class [[nodiscard]] imported_function_ordinal : public imported_function_address<Va>
{
public:
	[[nodiscard]]
	ordinal_type get_ordinal() const noexcept
	{
		return ordinal_;
	}

	void set_ordinal(ordinal_type ordinal) noexcept
	{
		ordinal_ = ordinal;
	}

	[[nodiscard]]
	Va to_thunk() const noexcept
	{
		auto result = static_cast<Va>(ordinal_);
		if constexpr (std::is_same_v<Va, std::uint32_t>)
			result |= detail::imports::image_ordinal_flag32;
		else
			result |= detail::imports::image_ordinal_flag64;
		return result;
	}

private:
	ordinal_type ordinal_{};
};

template<detail::executable_pointer Va>
class [[nodiscard]] imported_function_hint_and_name : public imported_function_address<Va>
{
public:
	using hint_type = packed_struct<std::uint16_t>;
	using name_type = packed_c_string;

public:
	[[nodiscard]]
	hint_type& get_hint() noexcept
	{
		return hint_;
	}

	[[nodiscard]]
	const hint_type& get_hint() const noexcept
	{
		return hint_;
	}

	[[nodiscard]]
	name_type& get_name() & noexcept
	{
		return name_;
	}

	[[nodiscard]]
	name_type get_name() && noexcept
	{
		return std::move(name_);
	}

	[[nodiscard]]
	const name_type& get_name() const & noexcept
	{
		return name_;
	}

private:
	hint_type hint_;
	name_type name_;
};

template<detail::executable_pointer Va>
class [[nodiscard]] imported_address
{
public:
	using va_type = Va;
	using packed_va_type = packed_struct<va_type>;
	using optional_va_type = std::optional<packed_va_type>;
	
	using ordinal_type = imported_function_ordinal<Va>;
	using hint_name_type = imported_function_hint_and_name<Va>;
	using imported_function_address_type = imported_function_address<va_type>;
	using import_info_type = std::variant<imported_function_address_type,
		ordinal_type, hint_name_type>;

public:
	[[nodiscard]]
	optional_va_type& get_lookup() noexcept
	{
		return lookup_entry_;
	}

	[[nodiscard]]
	const optional_va_type& get_lookup() const noexcept
	{
		return lookup_entry_;
	}
	
	[[nodiscard]]
	packed_va_type& get_address() noexcept
	{
		return address_entry_;
	}

	[[nodiscard]]
	const packed_va_type& get_address() const noexcept
	{
		return address_entry_;
	}

	[[nodiscard]]
	import_info_type& get_import_info() noexcept
	{
		return import_info_;
	}

	[[nodiscard]]
	const import_info_type& get_import_info() const noexcept
	{
		return import_info_;
	}

private:
	optional_va_type lookup_entry_;
	packed_va_type address_entry_;
	import_info_type import_info_;
};

template<detail::executable_pointer Va>
class [[nodiscard]] imported_address_details
	: public imported_address<Va>
	, public error_list
{
};

template<detail::executable_pointer Va>
using imported_address_list = std::vector<imported_address<Va>>;
template<detail::executable_pointer Va>
using imported_address_details_list = std::vector<imported_address_details<Va>>;

} //namespace pe_bliss::imports
