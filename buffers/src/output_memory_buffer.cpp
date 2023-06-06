#include "buffers/output_memory_buffer.h"

#include <algorithm>
#include <cstddef>
#include <system_error>

#include "utilities/generic_error.h"
#include "utilities/math.h"

namespace buffers
{

std::size_t output_memory_buffer::size()
{
	return data_.size();
}

void output_memory_buffer::write(std::size_t count, const std::byte* data)
{
	if (pos_ + count > data_.size())
		data_.resize(pos_ + count);

	std::copy(data, data + count, data_.data() + pos_);
	pos_ += count;
}

void output_memory_buffer::set_wpos(std::size_t pos)
{
	if (pos >= data_.max_size())
		throw std::system_error(utilities::generic_errc::buffer_overrun);

	if (pos > data_.size())
		data_.resize(pos);

	pos_ = pos;
}

void output_memory_buffer::advance_wpos(std::int32_t offset)
{
	auto new_pos = pos_;
	if (!utilities::math::add_offset_if_safe(new_pos, offset))
		throw std::system_error(utilities::generic_errc::buffer_overrun);
	set_wpos(new_pos);
}

std::size_t output_memory_buffer::wpos()
{
	return pos_;
}

} //namespace buffers
