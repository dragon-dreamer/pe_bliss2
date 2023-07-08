#pragma once

#include <cstddef>
#include <vector>

#include "buffers/input_buffer_interface.h"

namespace buffers
{

class [[nodiscard]] input_container_buffer final
	: public input_buffer_interface
{
public:
	using container_type = std::vector<std::byte>;

public:
	input_container_buffer(std::size_t absolute_offset = 0,
		std::size_t relative_offset = 0);

	[[nodiscard]]
	virtual std::size_t size() override;
	[[nodiscard]]
	virtual const std::byte* get_raw_data(std::size_t pos, std::size_t count) const override;

	virtual std::size_t read(std::size_t pos,
		std::size_t count, std::byte* data) override;

	[[nodiscard]]
	container_type& get_container() noexcept
	{
		return container_;
	}

	[[nodiscard]]
	const container_type& get_container() const noexcept
	{
		return container_;
	}

private:
	container_type container_;
};

} //namespace buffers
