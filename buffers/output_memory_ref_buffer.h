#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "buffers/output_buffer_interface.h"

namespace buffers
{

class output_memory_ref_buffer : public output_buffer_interface
{
public:
	output_memory_ref_buffer(std::byte* data, std::size_t size) noexcept
		: data_(data)
		, size_(size)
	{
	}

	template<std::size_t Extent>
	output_memory_ref_buffer(std::span<std::byte, Extent> data) noexcept
		: data_(data.data())
		, size_(data.size_bytes())
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
	std::byte* data_;
	std::size_t size_;
	std::size_t pos_ = 0;
};

} //namespace buffers
