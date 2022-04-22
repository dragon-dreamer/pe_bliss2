#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>

#include "buffers/output_buffer_interface.h"

namespace buffers
{

class [[nodiscard]] output_stream_buffer : public output_buffer_interface
{
public:
	explicit output_stream_buffer(std::ostream& stream);

	[[nodiscard]]
	virtual std::size_t size() override;

	virtual void write(std::size_t count, const std::byte* data) override;
	virtual void set_wpos(std::size_t pos) override;
	virtual void advance_wpos(std::int32_t offset) override;
	[[nodiscard]]
	virtual std::size_t wpos() override;

	[[nodiscard]] std::ostream& get_stream() noexcept
	{
		return stream_;
	}

	[[nodiscard]] const std::ostream& get_stream() const noexcept
	{
		return stream_;
	}

private:
	std::ostream& stream_;
};

} //namespace buffers
