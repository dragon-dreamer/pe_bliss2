#pragma once

namespace utilities
{

struct static_class
{
	static_class() = delete;
	static_class(const static_class&) = delete;
	static_class& operator=(const static_class&) = delete;
};

} //namespace utilities
