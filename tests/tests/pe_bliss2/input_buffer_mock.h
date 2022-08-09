#pragma once

#include "gmock/gmock.h"

#include "buffers/input_buffer_interface.h"

class input_buffer_mock : public buffers::input_buffer_interface
{
public:
	MOCK_METHOD(std::size_t, size, (), (override));
	MOCK_METHOD(bool, is_stateless, (), (const, noexcept, override));
	MOCK_METHOD(std::size_t, virtual_size, (), (const, noexcept, override));
	MOCK_METHOD(std::size_t, read, (std::size_t pos, std::size_t count, std::byte* data), (override));
};
