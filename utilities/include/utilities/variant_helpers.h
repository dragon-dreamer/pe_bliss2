#pragma once

namespace utilities
{

template<typename... Ts>
struct [[nodiscard]] overloaded : Ts...
{
	using Ts::operator()...;
};

} //namespace utilities
