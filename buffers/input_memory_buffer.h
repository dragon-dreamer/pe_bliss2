#pragma once

#include <cstddef>
#include <cstdint>

#include "buffers/input_buffer_interface.h"

namespace buffers
{

class [[nodiscard]] input_memory_buffer : public input_buffer_interface
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

private:
	const std::byte* memory_;
	std::size_t size_;
	std::size_t pos_;
};

} //namespace buffers
