#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "buffers/buffer_interface.h"

namespace buffers
{

class input_buffer_interface : public buffer_interface
{
public:
	virtual std::size_t read(std::size_t count, std::byte* data) = 0;
	virtual void set_rpos(std::size_t pos) = 0;
	virtual void advance_rpos(std::int32_t offset) = 0;
	[[nodiscard]]
	virtual std::size_t rpos() = 0;
	[[nodiscard]]
	virtual std::size_t absolute_offset() const noexcept = 0;
	[[nodiscard]]
	virtual std::size_t relative_offset() const noexcept = 0;
};

using input_buffer_ptr = std::shared_ptr<input_buffer_interface>;

} //namespace buffers
