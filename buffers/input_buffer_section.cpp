#include "buffers/input_buffer_section.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <memory>
#include <system_error>
#include <utility>

#include "utilities/generic_error.h"
#include "utilities/math.h"

namespace buffers
{

input_buffer_section::input_buffer_section(input_buffer_ptr buf,
	std::size_t offset, std::size_t size)
	: buf_(std::move(buf))
	, offset_(offset)
	, size_(size)
	, pos_(0)
{
	assert(!!buf_);

	auto max_offset = offset;
	if (!utilities::math::add_if_safe(max_offset, size))
		throw std::system_error(utilities::generic_errc::integer_overflow);
	if (max_offset > buf_->size())
		throw std::system_error(utilities::generic_errc::buffer_overrun);

	parent_absolute_offset_ = buf_->absolute_offset();
	relative_offset_ = offset_ + buf_->relative_offset();
}

input_buffer_section input_buffer_section::reduce(
	std::size_t offset, std::size_t size) const
{
	std::size_t total_offset = offset_;
	if (!utilities::math::add_if_safe(total_offset, offset))
		throw std::system_error(utilities::generic_errc::integer_overflow);

	if (!utilities::math::is_sum_safe(total_offset, size))
		throw std::system_error(utilities::generic_errc::integer_overflow);

	if (total_offset + size > offset_ + size_)
		throw std::system_error(utilities::generic_errc::buffer_overrun);

	return input_buffer_section(buf_, total_offset, size);
}

std::size_t input_buffer_section::read(std::size_t count, std::byte* data)
{
	count = (std::min)(count, size_ - pos_);
	if (!count)
		return 0u;
	buf_->set_rpos(offset_ + pos_);
	auto read_bytes = buf_->read(count, data);
	pos_ += read_bytes;
	return read_bytes;
}

void input_buffer_section::set_rpos(std::size_t pos)
{
	if (pos > size_)
		throw std::system_error(utilities::generic_errc::buffer_overrun);

	pos_ = pos;
}

void input_buffer_section::advance_rpos(std::int32_t offset)
{
	auto new_pos = pos_;
	if (!utilities::math::add_offset_if_safe(new_pos, offset))
		throw std::system_error(utilities::generic_errc::buffer_overrun);
	set_rpos(new_pos);
}

std::size_t input_buffer_section::rpos()
{
	return pos_;
}

std::size_t input_buffer_section::size()
{
	return size_;
}

std::size_t input_buffer_section::relative_offset() const noexcept
{
	return relative_offset_;
}

std::size_t input_buffer_section::absolute_offset() const noexcept
{
	return offset_ + parent_absolute_offset_;
}

input_buffer_ptr reduce(const input_buffer_ptr& buf,
	std::size_t offset, std::size_t size)
{
	return std::make_shared<input_buffer_section>(buf, offset, size);
}

input_buffer_ptr reduce(const input_buffer_ptr& buf,
	std::size_t offset)
{
	auto size = buf->size();
	if (offset > size)
		throw std::system_error(utilities::generic_errc::buffer_overrun);
	return reduce(buf, offset, size - offset);
}

} //namespace buffers
