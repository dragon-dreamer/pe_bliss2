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
	, virtual_size_{}
{
	assert(!!buf_);

	auto max_offset = offset;
	if (!utilities::math::add_if_safe(max_offset, size))
		throw std::system_error(utilities::generic_errc::integer_overflow);
	if (max_offset > buf_->size())
		throw std::system_error(utilities::generic_errc::buffer_overrun);

	auto absolute_offset = offset_;
	if (!utilities::math::add_if_safe(absolute_offset, buf_->absolute_offset()))
		throw std::system_error(utilities::generic_errc::integer_overflow);
	auto relative_offset = offset_;
	if (!utilities::math::add_if_safe(relative_offset, buf_->relative_offset()))
		throw std::system_error(utilities::generic_errc::integer_overflow);
	set_absolute_offset(absolute_offset);
	set_relative_offset(relative_offset);

	auto src_physical_size = buf_->physical_size();
	auto dst_last_offset = offset_ + size_;
	if (dst_last_offset > src_physical_size)
		virtual_size_ = std::min(dst_last_offset - src_physical_size, size_);
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

std::size_t input_buffer_section::read(std::size_t pos,
	std::size_t count, std::byte* data)
{
	if (!count)
		return 0u;

	if (!utilities::math::is_sum_safe(pos, count) || pos + count > size_)
		throw std::system_error(utilities::generic_errc::buffer_overrun);

	return buf_->read(pos + offset_, count, data);
}

const std::byte* input_buffer_section::get_raw_data(std::size_t pos, std::size_t count) const
{
	if (!utilities::math::is_sum_safe(pos, count) || pos + count > size_)
		throw std::system_error(utilities::generic_errc::buffer_overrun);

	return buf_->get_raw_data(pos + offset_, count);
}

std::size_t input_buffer_section::size()
{
	return size_;
}

bool input_buffer_section::is_stateless() const noexcept
{
	return buf_->is_stateless();
}

std::size_t input_buffer_section::virtual_size() const noexcept
{
	return virtual_size_;
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
