#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace pe_bliss::security
{

using span_range_type = std::span<const std::byte>;
using vector_range_type = std::vector<std::byte>;

} //namespace pe_bliss::security
