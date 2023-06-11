#pragma once

namespace utilities
{

template<typename... T>
struct [[nodiscard]] overloaded : T...
{
	using T::operator()...;
};

template<typename... T>
overloaded(T...) -> overloaded<T...>;

} //namespace utilities
