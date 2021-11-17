#include "buffers/buffer_copy.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>

#include "buffers/input_buffer_interface.h"
#include "buffers/output_buffer_interface.h"

namespace buffers
{

void copy(input_buffer_interface& from,
	output_buffer_interface& to, std::size_t size)
{
	static constexpr std::size_t temp_buffer_size = 512;
	std::array<std::byte, temp_buffer_size> temp;
	while (size)
	{
		auto count = (std::min)(size, temp_buffer_size);
		from.read(count, temp.data());
		to.write(count, temp.data());
		size -= count;
	}
}

} //namespace buffers
