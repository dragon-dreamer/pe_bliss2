#include "buffers/input_virtual_buffer.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <system_error>
#include <utility>

#include "utilities/generic_error.h"
#include "utilities/math.h"

namespace buffers
{

input_virtual_buffer::input_virtual_buffer(
	input_buffer_ptr buf, std::size_t additional_virtual_size)
	: buf_(std::move(buf))
	, additional_virtual_size_(additional_virtual_size)
{
	assert(!!buf_);

	set_absolute_offset(buf_->absolute_offset());
	set_relative_offset(buf_->relative_offset());
}

std::size_t input_virtual_buffer::read(std::size_t pos,
	std::size_t count, std::byte* data)
{
	if (!count)
		return 0u;

	auto size = buf_->size();

	if (!utilities::math::is_sum_safe(additional_virtual_size_, size))
		throw std::system_error(utilities::generic_errc::integer_overflow);

	if (!utilities::math::is_sum_safe(pos, count)
		|| pos + count > size + additional_virtual_size_)
	{
		throw std::system_error(utilities::generic_errc::buffer_overrun);
	}
	
	std::size_t real_physical_bytes = 0;
	std::size_t underlying_virtual_bytes = 0;
	if (size > pos)
	{
		auto expected_physical_bytes = std::min(count, size - pos);
		real_physical_bytes = buf_->read(pos, expected_physical_bytes, data);
		data += expected_physical_bytes;
		underlying_virtual_bytes = expected_physical_bytes - real_physical_bytes;
	}
	std::memset(data, 0, count - real_physical_bytes - underlying_virtual_bytes);
	return real_physical_bytes;
}

const std::byte* input_virtual_buffer::get_raw_data(std::size_t pos, std::size_t count) const
{
	return buf_->get_raw_data(pos, count);
}

std::size_t input_virtual_buffer::size()
{
	return buf_->size() + additional_virtual_size_;
}

bool input_virtual_buffer::is_stateless() const noexcept
{
	return buf_->is_stateless();
}

std::size_t input_virtual_buffer::virtual_size() const noexcept
{
	return buf_->virtual_size() + additional_virtual_size_;
}

} //namespace buffers
