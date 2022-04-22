#pragma once

#include "gmock/gmock.h"

#include "buffers/output_buffer_interface.h"

class output_buffer_mock : public buffers::output_buffer_interface
{
public:
	MOCK_METHOD(std::size_t, size, ());
	MOCK_METHOD(void, write, (std::size_t count, const std::byte* data));
	MOCK_METHOD(void, set_wpos, (std::size_t pos));
	MOCK_METHOD(void, advance_wpos, (std::int32_t offset));
	MOCK_METHOD(std::size_t, wpos, ());
};
