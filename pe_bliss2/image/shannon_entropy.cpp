#include "pe_bliss2/image/shannon_entropy.h"

#include <algorithm>
#include <array>
#include <cstddef>

#include "buffers/input_buffer_interface.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "pe_bliss2/image/image.h"
#include "utilities/shannon_entropy.h"

namespace
{

void calculate_entropy_impl(utilities::shannon_entropy& entropy,
	buffers::input_buffer_interface& buf)
{
	buffers::input_buffer_stateful_wrapper_ref ref(buf);
	static constexpr std::size_t temp_buffer_size = 512;
	std::array<std::byte, temp_buffer_size> temp;
	auto count = buf.physical_size();
	while (count)
	{
		auto read_count = std::min(count, temp_buffer_size);
		ref.read(read_count, temp.data());
		count -= read_count;
		for (std::size_t i = 0; i != read_count; ++i)
			entropy.update(temp[i]);
	}
}

} //namespace

namespace pe_bliss::image
{

float calculate_shannon_entropy(const image& instance)
{
	utilities::shannon_entropy entropy;
	calculate_entropy_impl(entropy, *instance.get_full_headers_buffer().data());
	for (const auto& section : instance.get_section_data_list())
		calculate_entropy_impl(entropy, *section.data());
	calculate_entropy_impl(entropy, *instance.get_overlay().data());
	return entropy.finalize();
}

float calculate_shannon_entropy(buffers::input_buffer_interface& buf)
{
	utilities::shannon_entropy entropy;
	calculate_entropy_impl(entropy, buf);
	return entropy.finalize();
}

} //namespace pe_bliss::image
