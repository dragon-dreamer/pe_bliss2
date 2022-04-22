#include "buffers/input_buffer_state.h"

#include "buffers/input_buffer_interface.h"

namespace buffers
{

input_buffer_state::input_buffer_state(input_buffer_interface& buffer)
	: buffer_pos_(buffer.rpos())
	, absolute_offset_(buffer.absolute_offset())
	, relative_offset_(buffer.relative_offset())
{
}

serialized_data_state::serialized_data_state(input_buffer_interface& buffer)
	: input_buffer_state(buffer)
{
	absolute_offset_ += buffer_pos_;
	relative_offset_ += buffer_pos_;
}

} //namespace buffers
