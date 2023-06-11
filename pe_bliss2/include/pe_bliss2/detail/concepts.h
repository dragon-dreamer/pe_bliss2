#pragma once

#include <cstddef>
#include <cstdint>
#include <concepts>
#include <type_traits>

namespace pe_bliss::detail
{

template<typename T>
concept standard_layout = std::is_standard_layout_v<T>;

template<typename T>
concept byte_pointer = std::is_same_v<std::byte*, T> || std::is_same_v<const std::byte*, T>;

template<typename T>
concept executable_pointer = std::is_same_v<T, std::uint32_t> || std::is_same_v<T, std::uint64_t>;

template<typename T>
concept has_state_with_absolute_offset = requires(T val)
{
	{ val.get_state().absolute_offset() } -> std::convertible_to<std::uint32_t>;
};

} //namespace pe_bliss::detail
