#pragma once

#include <cstdint>
#include <type_traits>

namespace pe_bliss::detail
{

template<typename T>
concept standard_layout = std::is_standard_layout_v<T>;

template<typename T>
concept byte_pointer = std::is_same_v<std::byte*, T> || std::is_same_v<const std::byte*, T>;

template<typename T>
concept executable_pointer = std::is_same_v<T, std::uint32_t> || std::is_same_v<T, std::uint64_t>;

} //namespace pe_bliss::detail
