#pragma once

#include <cstddef>

#include "buffers/input_buffer_interface.h"

namespace buffers
{

class [[nodiscard]] input_buffer_section final
	: public input_buffer_interface
{
public:
	input_buffer_section(input_buffer_ptr buf,
		std::size_t offset, std::size_t size);

	[[nodiscard]]
	virtual bool is_stateless() const noexcept override;
	[[nodiscard]]
	virtual std::size_t virtual_size() const noexcept override;

	virtual std::size_t read(std::size_t pos,
		std::size_t count, std::byte* data) override;

	[[nodiscard]]
	virtual std::size_t size() override;

	[[nodiscard]]
	input_buffer_section reduce(std::size_t offset, std::size_t size) const;

private:
	input_buffer_ptr buf_;
	std::size_t offset_;
	std::size_t size_;
	std::size_t virtual_size_;
};

[[nodiscard]]
input_buffer_ptr reduce(const input_buffer_ptr& buf,
	std::size_t offset, std::size_t size);

[[nodiscard]]
input_buffer_ptr reduce(const input_buffer_ptr& buf,
	std::size_t offset);

} //namespace buffers
