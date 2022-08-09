#include "buffers/buffer_copy.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/output_buffer_interface.h"

namespace buffers
{

std::size_t copy(input_buffer_stateful_wrapper_ref& from,
	output_buffer_interface& to, std::size_t size)
{
	static constexpr std::size_t temp_buffer_size = 512;
	std::array<std::byte, temp_buffer_size> temp;
	auto initial_size = size;
	auto count = size;
	while (size && count)
	{
		count = (std::min)(size, temp_buffer_size);
		from.read(count, temp.data());
		to.write(count, temp.data());
		size -= count;
	}
	return initial_size - size;
}

std::size_t copy(input_buffer_interface& from,
	output_buffer_interface& to, std::size_t size)
{
	input_buffer_stateful_wrapper_ref ref(from);
	return copy(ref, to, size);
}

} //namespace buffers
