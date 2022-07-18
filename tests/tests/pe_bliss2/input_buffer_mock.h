#pragma once

#include "gmock/gmock.h"

#include "buffers/input_buffer_interface.h"

class input_buffer_mock : public buffers::input_buffer_interface
{
public:
	MOCK_METHOD(std::size_t, size, ());
	MOCK_METHOD(std::size_t, read, (std::size_t count, std::byte* data));
	MOCK_METHOD(void, set_rpos, (std::size_t pos));
	MOCK_METHOD(void, advance_rpos, (std::int32_t offset));
	MOCK_METHOD(std::size_t, rpos, ());
};
