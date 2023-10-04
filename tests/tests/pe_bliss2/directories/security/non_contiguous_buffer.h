#pragma once

#include <cstddef>

#include "buffers/input_buffer_interface.h"
#include "buffers/input_container_buffer.h"

class non_contiguous_buffer final : public buffers::input_buffer_interface
{
public:
	using container_type = buffers::input_container_buffer::container_type;

public:
	non_contiguous_buffer(std::size_t absolute_offset = 0,
		std::size_t relative_offset = 0)
		: container_(absolute_offset, relative_offset)
	{
	}

	virtual std::size_t size() override
	{
		return container_.size();
	}

	virtual std::size_t read(std::size_t pos,
		std::size_t count, std::byte* data) override
	{
		return container_.read(pos, count, data);
	}

	[[nodiscard]]
	container_type& get_container() noexcept
	{
		return container_.get_container();
	}

	[[nodiscard]]
	const container_type& get_container() const noexcept
	{
		return container_.get_container();
	}

private:
	buffers::input_container_buffer container_;
};
