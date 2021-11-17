#pragma once

#include <cstddef>
#include <cstdint>

#include "buffers/buffer_interface.h"

namespace buffers
{

class output_buffer_interface : public buffer_interface
{
public:
	virtual void write(std::size_t count, const std::byte* data) = 0;
	virtual void set_wpos(std::size_t pos) = 0;
	virtual void advance_wpos(std::int32_t offset) = 0;
	[[nodiscard]]
	virtual std::size_t wpos() = 0;
};

} //namespace buffers
