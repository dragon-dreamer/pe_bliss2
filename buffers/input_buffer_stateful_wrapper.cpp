#include "buffers/input_buffer_stateful_wrapper.h"

#include <system_error>
#include <utility>

#include "utilities/math.h"
#include "utilities/generic_error.h"

namespace buffers
{
input_buffer_stateful_wrapper_ref::input_buffer_stateful_wrapper_ref(
	input_buffer_interface& buf) noexcept
	: buf_(buf)
	, pos_{}
{
}

std::size_t input_buffer_stateful_wrapper_ref::size()
{
	return buf_.size();
}

std::size_t input_buffer_stateful_wrapper_ref::read(std::size_t count, std::byte* data)
{
	auto physical_bytes_read = buf_.read(pos_, count, data);
	pos_ += count;
	return physical_bytes_read;
}

void input_buffer_stateful_wrapper_ref::set_rpos(std::size_t pos)
{
	if (pos > size())
		throw std::system_error(utilities::generic_errc::buffer_overrun);
	pos_ = pos;
}

void input_buffer_stateful_wrapper_ref::advance_rpos(std::int32_t offset)
{
	auto new_pos = pos_;
	if (!utilities::math::add_offset_if_safe(new_pos, offset))
		throw std::system_error(utilities::generic_errc::buffer_overrun);
	set_rpos(new_pos);
}

std::size_t input_buffer_stateful_wrapper_ref::rpos() const noexcept
{
	return pos_;
}

const input_buffer_interface& input_buffer_stateful_wrapper_ref::get_buffer() const noexcept
{
	return buf_;
}

input_buffer_interface& input_buffer_stateful_wrapper_ref::get_buffer() noexcept
{
	return buf_;
}

input_buffer_stateful_wrapper::input_buffer_stateful_wrapper(
	const input_buffer_ptr& buf) noexcept
	: input_buffer_stateful_wrapper_ref(*buf)
	, buf_(buf)
{
}

input_buffer_stateful_wrapper::input_buffer_stateful_wrapper(
	input_buffer_ptr&& buf) noexcept
	: input_buffer_stateful_wrapper_ref(*buf)
	, buf_(std::move(buf))
{
}

const input_buffer_ptr& input_buffer_stateful_wrapper::get_buffer() const noexcept
{
	return buf_;
}
} //namespace buffers
