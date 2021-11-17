#pragma once

#include <cstddef>
#include <cstdint>

#include "buffers/output_buffer_interface.h"

namespace buffers
{

class fixed_output_memory_buffer : public output_buffer_interface
{
public:
	fixed_output_memory_buffer(std::byte* memory, std::size_t size) noexcept;

	[[nodiscard]]
	virtual std::size_t size() override;

	virtual void write(std::size_t count, const std::byte* data) override;
	virtual void set_wpos(std::size_t pos) override;
	virtual void advance_wpos(std::int32_t offset) override;
	[[nodiscard]]
	virtual std::size_t wpos() override;

private:
	std::byte* memory_;
	std::size_t size_;
	std::size_t pos_;
};

} //namespace buffers
