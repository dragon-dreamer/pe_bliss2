#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "buffers/output_buffer_interface.h"

namespace buffers
{

class [[nodiscard]] output_memory_buffer : public output_buffer_interface
{
public:
	using buffer_type = std::vector<std::byte>;

public:
	explicit output_memory_buffer(buffer_type& data) noexcept
		: data_(data)
	{
	}

	[[nodiscard]]
	virtual std::size_t size() override;

	virtual void write(std::size_t count, const std::byte* data) override;
	virtual void set_wpos(std::size_t pos) override;
	virtual void advance_wpos(std::int32_t offset) override;
	[[nodiscard]]
	virtual std::size_t wpos() override;

private:
	buffer_type& data_;
	std::size_t pos_ = 0;
};

} //namespace buffers
