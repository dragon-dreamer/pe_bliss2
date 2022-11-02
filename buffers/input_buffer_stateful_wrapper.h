#pragma once

#include <cstddef>

#include "buffers/input_buffer_interface.h"

namespace buffers
{
class [[nodiscard]] input_buffer_stateful_wrapper_ref
{
public:
	input_buffer_stateful_wrapper_ref(input_buffer_interface& buf) noexcept;

	[[nodiscard]]
	std::size_t size();

	std::size_t read(std::size_t count, std::byte* data);

	void set_rpos(std::size_t pos);
	void advance_rpos(std::int32_t offset);
	[[nodiscard]]
	std::size_t rpos() const noexcept;

	[[nodiscard]]
	const input_buffer_interface& get_buffer() const noexcept;
	[[nodiscard]]
	input_buffer_interface& get_buffer() noexcept;

private:
	input_buffer_interface& buf_;
	std::size_t pos_;
};

class [[nodiscard]] input_buffer_stateful_wrapper
	: public input_buffer_stateful_wrapper_ref
{
public:
	input_buffer_stateful_wrapper(const input_buffer_ptr& buf) noexcept;
	input_buffer_stateful_wrapper(input_buffer_ptr&& buf) noexcept;

	const input_buffer_ptr& get_buffer() const noexcept;

private:
	input_buffer_ptr buf_;
};
} //namespace buffers
