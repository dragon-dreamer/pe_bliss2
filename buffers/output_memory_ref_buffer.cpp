#include "buffers/output_memory_ref_buffer.h"

#include <algorithm>
#include <cstring>
#include <system_error>

#include "utilities/generic_error.h"
#include "utilities/math.h"

namespace buffers
{

std::size_t output_memory_ref_buffer::size()
{
	return size_;
}

void output_memory_ref_buffer::write(std::size_t count, const std::byte* data)
{
	if (count > size_ - pos_)
		throw std::system_error(utilities::generic_errc::buffer_overrun);

	std::memcpy(data_ + pos_, data, count);
	pos_ += count;
}

void output_memory_ref_buffer::set_wpos(std::size_t pos)
{
	if (pos > size_)
		throw std::system_error(utilities::generic_errc::buffer_overrun);

	pos_ = pos;
}

void output_memory_ref_buffer::advance_wpos(std::int32_t offset)
{
	auto new_pos = pos_;
	if (!utilities::math::add_offset_if_safe(new_pos, offset))
		throw std::system_error(utilities::generic_errc::buffer_overrun);

	set_wpos(new_pos);
}

std::size_t output_memory_ref_buffer::wpos()
{
	return pos_;
}

} //namespace buffers
