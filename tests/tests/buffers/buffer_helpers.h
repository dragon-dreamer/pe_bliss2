#pragma once

#include <cstddef>
#include <memory>
#include <sstream>

#include "buffers/input_container_buffer.h"

inline auto create_stream(std::size_t size)
{
	auto result = std::make_shared<std::stringstream>();
	for (std::size_t i = 0; i != size; ++i)
		*result << static_cast<char>(i);

	return result;
}

inline auto create_input_container_buffer(std::size_t size)
{
	auto buf = std::make_shared<buffers::input_container_buffer>(0u, 0u);
	auto& container = buf->get_container();
	container.resize(size);
	for (std::size_t i = 0; i != size; ++i)
		container[i] = static_cast<std::byte>(i);

	return buf;
}
