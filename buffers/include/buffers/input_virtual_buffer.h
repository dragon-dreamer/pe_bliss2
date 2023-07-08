#pragma once

#include <cstddef>

#include "buffers/input_buffer_interface.h"

namespace buffers
{

class [[nodiscard]] input_virtual_buffer final
	: public input_buffer_interface
{
public:
	input_virtual_buffer(input_buffer_ptr buf,
		std::size_t additional_virtual_size);

	[[nodiscard]]
	virtual bool is_stateless() const noexcept override;
	[[nodiscard]]
	virtual const std::byte* get_raw_data(std::size_t pos, std::size_t count) const override;
	[[nodiscard]]
	virtual std::size_t virtual_size() const noexcept override;

	virtual std::size_t read(std::size_t pos,
		std::size_t count, std::byte* data) override;

	[[nodiscard]]
	virtual std::size_t size() override;

private:
	input_buffer_ptr buf_;
	std::size_t additional_virtual_size_;
};

} //namespace buffers
