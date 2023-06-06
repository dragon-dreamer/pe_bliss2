#pragma once

#include <cstdint>

#include "pe_bliss2/detail/concepts.h"

namespace pe_bliss::detail::tls
{

template<executable_pointer Pointer>
struct image_tls_directory
{
	using pointer_type = Pointer;

	Pointer start_address_of_raw_data;
	Pointer end_address_of_raw_data;
	Pointer address_of_index;
	Pointer address_of_callbacks;
	std::uint32_t size_of_zero_fill;
	std::uint32_t characteristics;
};

using image_tls_directory32 = image_tls_directory<std::uint32_t>;
using image_tls_directory64 = image_tls_directory<std::uint64_t>;

} //namespace pe_bliss::detail::tls
