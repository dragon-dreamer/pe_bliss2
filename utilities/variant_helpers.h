#pragma once

namespace utilities
{

template<typename... Ts>
struct overloaded : Ts...
{
	using Ts::operator()...;
};

} //namespace utilities
