#include "pe_bliss2/data_directories.h"

#include <algorithm>
#include <array>
#include <iterator>

#include "buffers/input_buffer_interface.h"
#include "buffers/output_buffer_interface.h"

namespace pe_bliss
{

void data_directories::deserialize(buffers::input_buffer_interface& buf,
	std::uint32_t number_of_rva_and_sizes, bool allow_virtual_memory)
{
	directories_.clear();
	if (!number_of_rva_and_sizes)
		return;

	directories_.resize(number_of_rva_and_sizes);
	for (std::uint32_t i = 0; i != number_of_rva_and_sizes; ++i)
	{
		directories_[i].deserialize(buf, allow_virtual_memory);
		if (directories_[i].is_virtual())
			break;
	}
}

void data_directories::serialize(buffers::output_buffer_interface& buf,
	bool write_virtual_part) const
{
	for (const auto& dir : directories_)
		dir.serialize(buf, write_virtual_part);
}

std::uint32_t data_directories::strip_data_directories(std::uint32_t min_count)
{
	std::uint32_t current_count = static_cast<std::uint32_t>(directories_.size());
	if (current_count <= min_count)
		return current_count;

	auto last_existing_it =
		std::find_if(std::crbegin(directories_), std::crend(directories_),
			[&current_count, min_count] (const auto& dir) mutable
			{
				return dir->virtual_address != 0u || current_count-- <= min_count;
			});

	auto remove_amount = std::distance(std::crbegin(directories_), last_existing_it);
	directories_.resize(directories_.size() - remove_amount);
	return static_cast<std::uint32_t>(directories_.size());
}

} //namespace pe_bliss
