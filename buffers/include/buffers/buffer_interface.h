#pragma once

#include <cstddef>

namespace buffers
{

class [[nodiscard]] buffer_interface
{
public:
	virtual ~buffer_interface() = default;

public:
	[[nodiscard]]
	virtual std::size_t size() = 0;
};

} //namespace buffers
