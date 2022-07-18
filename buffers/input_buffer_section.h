#pragma once

#include <cstddef>
#include <cstdint>

#include "buffers/input_buffer_interface.h"

namespace buffers
{

class [[nodiscard]] input_buffer_section : public input_buffer_interface
{
public:
	input_buffer_section(input_buffer_ptr buf,
		std::size_t offset, std::size_t size);

	input_buffer_section(const input_buffer_section&) noexcept = default;
	input_buffer_section(input_buffer_section&&) noexcept = default;
	input_buffer_section& operator=(const input_buffer_section&) noexcept = default;
	input_buffer_section& operator=(input_buffer_section&&) noexcept = default;

	virtual std::size_t read(std::size_t count, std::byte* data) override;
	virtual void set_rpos(std::size_t pos) override;
	virtual void advance_rpos(std::int32_t offset) override;
	[[nodiscard]]
	virtual std::size_t rpos() override;
	[[nodiscard]]
	virtual std::size_t size() override;

	[[nodiscard]]
	input_buffer_section reduce(std::size_t offset, std::size_t size) const;

private:
	input_buffer_ptr buf_;
	std::size_t offset_;
	std::size_t size_;
	std::size_t pos_;
};

[[nodiscard]]
input_buffer_ptr reduce(const input_buffer_ptr& buf,
	std::size_t offset, std::size_t size);

[[nodiscard]]
input_buffer_ptr reduce(const input_buffer_ptr& buf,
	std::size_t offset);

} //namespace buffers
