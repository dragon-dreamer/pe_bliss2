#pragma once

#include <cstddef>
#include <memory>

#include "buffers/buffer_interface.h"

namespace buffers
{

class [[nodiscard]] input_buffer_interface : public buffer_interface
{
public:
	[[nodiscard]]
	virtual bool is_stateless() const noexcept { return true; }

	[[nodiscard]]
	virtual std::size_t virtual_size() const noexcept { return 0u; }

	[[nodiscard]]
	std::size_t physical_size() { return size() - virtual_size(); }

	virtual std::size_t read(std::size_t pos, std::size_t count, std::byte* data) = 0;
	
	[[nodiscard]]
	std::size_t absolute_offset() const noexcept
	{
		return absolute_offset_;
	}

	[[nodiscard]]
	std::size_t relative_offset() const noexcept
	{
		return relative_offset_;
	}

	void set_absolute_offset(std::size_t absolute_offset) noexcept
	{
		absolute_offset_ = absolute_offset;
	}

	void set_relative_offset(std::size_t relative_offset) noexcept
	{
		relative_offset_ = relative_offset;
	}

private:
	std::size_t absolute_offset_{};
	std::size_t relative_offset_{};
};

using input_buffer_ptr = std::shared_ptr<input_buffer_interface>;

} //namespace buffers
