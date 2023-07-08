#pragma once

#include <cstddef>

#include "buffers/input_buffer_interface.h"

namespace buffers
{

class [[nodiscard]] input_memory_buffer final
	: public input_buffer_interface
{
public:
	input_memory_buffer(const std::byte* memory, std::size_t size) noexcept;

	[[nodiscard]]
	virtual const std::byte* get_raw_data(std::size_t pos, std::size_t count) const override;
	[[nodiscard]]
	virtual std::size_t size() override;

	virtual std::size_t read(std::size_t pos,
		std::size_t count, std::byte* data) override;

private:
	const std::byte* memory_;
	std::size_t size_;
};

} //namespace buffers
