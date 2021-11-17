#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>

#include "buffers/input_buffer_interface.h"

namespace buffers
{

class input_stream_buffer : public input_buffer_interface
{
public:
	explicit input_stream_buffer(const std::shared_ptr<std::istream>& stream);

	[[nodiscard]]
	virtual std::size_t size() override;

	virtual std::size_t read(std::size_t count, std::byte* data) override;

	virtual void set_rpos(std::size_t pos) override;
	virtual void advance_rpos(std::int32_t offset) override;

	[[nodiscard]]
	virtual std::size_t rpos() override;

	[[nodiscard]]
	virtual std::size_t absolute_offset() const noexcept override
	{
		return 0;
	}

	[[nodiscard]]
	virtual std::size_t relative_offset() const noexcept override
	{
		return 0;
	}

private:
	std::shared_ptr<std::istream> stream_;
};

} //namespace buffers
