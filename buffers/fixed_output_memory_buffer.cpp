#include "buffers/fixed_output_memory_buffer.h"

#include <cassert>
#include <cstring>
#include <system_error>

#include "utilities/generic_error.h"
#include "utilities/math.h"

namespace buffers
{

fixed_output_memory_buffer::fixed_output_memory_buffer(
	std::byte* memory, std::size_t size) noexcept
	: memory_(memory)
	, size_(size)
	, pos_(0)
{
	assert(!size_ || memory_);
}

std::size_t fixed_output_memory_buffer::size()
{
	return size_;
}

void fixed_output_memory_buffer::write(std::size_t count, const std::byte* data)
{
	if (count > size_ - pos_)
		throw std::system_error(utilities::generic_errc::buffer_overrun);

	std::memcpy(memory_ + pos_, data, count);
	pos_ += count;
}

void fixed_output_memory_buffer::set_wpos(std::size_t pos)
{
	if (pos >= size_)
		throw std::system_error(utilities::generic_errc::buffer_overrun);

	pos_ = pos;
}

void fixed_output_memory_buffer::advance_wpos(std::int32_t offset)
{
	auto new_pos = pos_;
	if (!utilities::math::add_offset_if_safe(new_pos, offset))
		throw std::system_error(utilities::generic_errc::buffer_overrun);
	set_wpos(new_pos);
}

std::size_t fixed_output_memory_buffer::wpos()
{
	return pos_;
}

} //namespace buffers
