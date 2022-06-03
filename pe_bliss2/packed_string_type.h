#pragma once

#include <type_traits>

namespace pe_bliss
{

class packed_c_string;
class packed_utf16_string;

template<typename T>
concept packed_string_type = std::is_same_v<T, packed_utf16_string>
	|| std::is_same_v<T, packed_c_string>;

} //namespace pe_bliss
