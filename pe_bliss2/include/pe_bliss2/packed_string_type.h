#pragma once

#include <type_traits>

#include "pe_bliss2/packed_c_string.h"

namespace pe_bliss
{

class packed_utf16_string;

template<typename T>
concept packed_string_type = std::is_same_v<T, packed_utf16_string>
	|| std::is_same_v<T, packed_c_string>
	|| std::is_same_v<T, packed_utf16_c_string>;

} //namespace pe_bliss
