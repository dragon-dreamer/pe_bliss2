#include "buffers/input_container_buffer.h"

#include <algorithm>

#include "utilities/generic_error.h"
#include "utilities/math.h"

namespace buffers
{

input_container_buffer::input_container_buffer(std::size_t absolute_offset,
	std::size_t relative_offset)
	: absolute_offset_(absolute_offset)
	, relative_offset_(relative_offset)
{
}

std::size_t input_container_buffer::size()
{
	return container_.size();
}

std::size_t input_container_buffer::read(std::size_t count, std::byte* data)
{
	if (pos_ > size()) //Container may be modified externally
		pos_ = size();

	count = (std::min)(count, size() - pos_);
	std::memcpy(data, container_.data() + pos_, count);
	pos_ += count;
	return count;
}

void input_container_buffer::set_rpos(std::size_t pos)
{
	if (pos > size())
		throw std::system_error(utilities::generic_errc::buffer_overrun);

	pos_ = pos;
}

void input_container_buffer::advance_rpos(std::int32_t offset)
{
	auto new_pos = pos_;
	if (!utilities::math::add_offset_if_safe(new_pos, offset))
		throw std::system_error(utilities::generic_errc::buffer_overrun);
	set_rpos(new_pos);
}

std::size_t input_container_buffer::rpos()
{
	return pos_;
}

std::size_t input_container_buffer::absolute_offset() const noexcept
{
	return absolute_offset_;
}

std::size_t input_container_buffer::relative_offset() const noexcept
{
	return relative_offset_;
}

} //namespace buffers
