#pragma once

#include <algorithm>
#include <string>
#include <string_view>

namespace utilities
{

[[nodiscard]]
constexpr char to_lower(char c) noexcept
{
	static_assert('F' == 70 && 'Z' == 90 && 'f' == 102 && 'z' == 122);
	if (c >= 'A' && c <= 'Z')
		c = 'a' + (c - 'A');
	return c;
}

[[nodiscard]]
constexpr bool char_iequal(char l, char r) noexcept
{
	return to_lower(l) == to_lower(r);
}

inline void to_lower_inplace(std::string& str) noexcept
{
	std::transform(str.begin(), str.end(), str.begin(), utilities::to_lower);
}

[[nodiscard]]
constexpr bool iequal(std::string_view l, std::string_view r) noexcept
{
	return std::equal(l.cbegin(), l.cend(),
		r.cbegin(), r.cend(), char_iequal);
}

constexpr void trim(std::string_view& str) noexcept
{
	while (!str.empty() && str[0] == ' ')
		str.remove_prefix(1);
	while (!str.empty() && str.back() == ' ')
		str.remove_suffix(1);
}

} //namespace utilities
