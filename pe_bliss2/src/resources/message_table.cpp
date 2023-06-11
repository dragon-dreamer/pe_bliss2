#include "pe_bliss2/resources/message_table.h"

#include "pe_bliss2/error_list.h"
#include "utilities/safe_uint.h"

namespace pe_bliss::resources
{

template<typename... Bases>
void message_block_base<Bases...>::set_start_id(id_type start_id) noexcept
{
	utilities::safe_uint end_id = start_id;
	end_id += entries_.size();
	this->descriptor_->low_id = start_id;
	this->descriptor_->high_id = end_id.value();
}

template class message_block_base<>;
template class message_block_base<error_list>;

} //namespace pe_bliss::resources
