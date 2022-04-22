#include "buffers/input_memory_buffer.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <system_error>

#include "utilities/generic_error.h"
#include "utilities/math.h"

namespace buffers
{

input_memory_buffer::input_memory_buffer(const std::byte* memory, std::size_t size) noexcept
	: memory_(memory)
	, size_(size)
	, pos_{}
{
	assert(!size_ || memory_);
}

std::size_t input_memory_buffer::size()
{
	return size_;
}

std::size_t input_memory_buffer::read(std::size_t count, std::byte* data)
{
	count = (std::min)(count, size_ - pos_);
	std::memcpy(data, memory_ + pos_, count);
	pos_ += count;
	return count;
}

void input_memory_buffer::advance_rpos(std::int32_t offset)
{
	auto new_pos = pos_;
	if (!utilities::math::add_offset_if_safe(new_pos, offset))
		throw std::system_error(utilities::generic_errc::buffer_overrun);
	set_rpos(new_pos);
}

void input_memory_buffer::set_rpos(std::size_t pos)
{
	if (pos > size_)
		throw std::system_error(utilities::generic_errc::buffer_overrun);

	pos_ = pos;
}

std::size_t input_memory_buffer::rpos()
{
	return pos_;
}

} //namespace buffers
