#pragma once

#include <cstddef>
#include <cstdint>

#include "buffers/input_buffer_interface.h"

namespace buffers
{

class input_memory_buffer : public input_buffer_interface
{
public:
	input_memory_buffer(const std::byte* memory, std::size_t size) noexcept;

	[[nodiscard]]
	virtual std::size_t size() override;

	virtual std::size_t read(std::size_t count, std::byte* data) override;

	virtual void set_rpos(std::size_t pos) override;
	virtual void advance_rpos(std::int32_t offset) override;

	[[nodiscard]]
	virtual std::size_t rpos() override;

	[[nodiscard]]
	virtual std::size_t absolute_offset() const noexcept override
	{
		return absolute_offset_;
	}

	[[nodiscard]]
	virtual std::size_t relative_offset() const noexcept override
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
	const std::byte* memory_;
	std::size_t size_;
	std::size_t pos_;
	std::size_t absolute_offset_{};
	std::size_t relative_offset_{};
};

} //namespace buffers
