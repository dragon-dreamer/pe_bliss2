#pragma once

#include <cstddef>
#include <iosfwd>
#include <memory>

#include "buffers/input_buffer_interface.h"

namespace buffers
{

class [[nodiscard]] input_stream_buffer final
	: public input_buffer_interface
{
public:
	explicit input_stream_buffer(std::shared_ptr<std::istream> stream);

	[[nodiscard]]
	virtual bool is_stateless() const noexcept override { return false; }

	[[nodiscard]]
	virtual std::size_t size() override;

	virtual std::size_t read(std::size_t pos,
		std::size_t count, std::byte* data) override;

	[[nodiscard]] const std::shared_ptr<std::istream>& get_stream() const noexcept
	{
		return stream_;
	}

private:
	std::shared_ptr<std::istream> stream_;
	std::size_t size_;
};

} //namespace buffers
