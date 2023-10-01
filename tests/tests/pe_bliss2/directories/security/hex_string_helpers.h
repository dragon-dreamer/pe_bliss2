#pragma once

#include <cassert>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

inline std::vector<std::byte> hex_string_to_bytes(std::string_view data)
{
	assert(!(data.size() % 2));
	std::vector<std::byte> result(data.size() / 2);
	const char* begin = data.data();
	for (std::size_t i = 0; i != data.size(); i += 2u, begin += 2u) {
		std::uint8_t value{};
		std::from_chars(begin, begin + 2u, value, 16u);
		result[i / 2u] = std::byte{ value };
	}
	return result;
}
