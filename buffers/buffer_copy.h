#pragma once

#include <cstddef>

namespace buffers
{

class input_buffer_interface;
class output_buffer_interface;

std::size_t copy(input_buffer_interface& from,
	output_buffer_interface& to, std::size_t size);

} //namespace buffers
