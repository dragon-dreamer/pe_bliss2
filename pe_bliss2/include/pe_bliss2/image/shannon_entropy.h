#pragma once

namespace buffers
{
class input_buffer_interface;
} //namespace buffers

namespace pe_bliss::image
{

class image;

[[nodiscard]]
float calculate_shannon_entropy(const image& instance);

[[nodiscard]]
float calculate_shannon_entropy(buffers::input_buffer_interface& buf);

} //namespace pe_bliss::image
