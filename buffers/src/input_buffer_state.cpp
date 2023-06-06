#include "buffers/input_buffer_state.h"

#include "buffers/input_buffer_stateful_wrapper.h"

namespace buffers
{

input_buffer_state::input_buffer_state(const input_buffer_stateful_wrapper_ref& wrapper)
	: buffer_pos_(wrapper.rpos())
	, absolute_offset_(wrapper.get_buffer().absolute_offset())
	, relative_offset_(wrapper.get_buffer().relative_offset())
{
}

serialized_data_state::serialized_data_state(const input_buffer_stateful_wrapper_ref& wrapper)
	: input_buffer_state(wrapper)
{
	absolute_offset_ += buffer_pos_;
	relative_offset_ += buffer_pos_;
}

} //namespace buffers
