#pragma once

#include <system_error>
#include <type_traits>

namespace utilities
{

enum class generic_errc
{
	integer_overflow = 1,
	buffer_overrun
};

std::error_code make_error_code(generic_errc) noexcept;

} //namespace utilities

namespace std
{
template<>
struct is_error_code_enum<utilities::generic_errc> : true_type {};
} //namespace std
