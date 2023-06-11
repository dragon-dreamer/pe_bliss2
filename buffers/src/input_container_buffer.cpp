#include "buffers/input_container_buffer.h"

#include <algorithm>
#include <cstring>

#include "utilities/generic_error.h"
#include "utilities/math.h"

namespace buffers
{

input_container_buffer::input_container_buffer(std::size_t absolute_offset,
	std::size_t relative_offset)
{
	set_absolute_offset(absolute_offset);
	set_relative_offset(relative_offset);
}

std::size_t input_container_buffer::size()
{
	return container_.size();
}

std::size_t input_container_buffer::read(std::size_t pos,
	std::size_t count, std::byte* data)
{
	if (!count)
		return 0u;

	if (!utilities::math::is_sum_safe(pos, count)
		|| pos + count > container_.size())
	{
		throw std::system_error(utilities::generic_errc::buffer_overrun);
	}

	std::memcpy(data, container_.data() + pos, count);
	return count;
}

} //namespace buffers
