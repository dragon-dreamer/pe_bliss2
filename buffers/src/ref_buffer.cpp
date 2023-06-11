#include "buffers/ref_buffer.h"

#include <cassert>
#include <memory>
#include <utility>
#include <variant>
#include <system_error>

#include "buffers/buffer_copy.h"
#include "buffers/input_buffer_interface.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_virtual_buffer.h"
#include "buffers/output_buffer_interface.h"
#include "utilities/generic_error.h"
#include "utilities/math.h"

namespace
{

struct ref_buffer_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "ref_buffer";
	}

	std::string message(int ev) const override
	{
		using enum buffers::ref_buffer_errc;
		switch (static_cast<buffers::ref_buffer_errc>(ev))
		{
		case unable_to_read_data:
			return "Unable to read data";
		case data_is_not_copied:
			return "Data is referenced and is not copied to local buffer";
		default:
			return {};
		}
	}
};

const ref_buffer_error_category ref_buffer_error_category_instance;

} //namespace

namespace buffers
{

std::error_code make_error_code(ref_buffer_errc e) noexcept
{
	return { static_cast<int>(e), ref_buffer_error_category_instance };
}

ref_buffer::ref_buffer()
{
	auto container = std::make_shared<input_container_buffer>();
	auto buf_ptr = container.get();
	buffer_.emplace<copied_buffer>(buf_ptr, std::move(container));
}

ref_buffer::ref_buffer(const ref_buffer& other)
{
	*this = other;
}

ref_buffer& ref_buffer::operator=(const ref_buffer& other)
{
	if (&other == this)
		return *this;

	buffer_.emplace<buffer_ref>(other.data());
	return *this;
}

void ref_buffer::deserialize(const input_buffer_ptr& buffer, bool copy_memory)
{
	assert(buffer);

	if (copy_memory)
		read_buffer(*buffer);
	else
		buffer_.emplace<buffer_ref>(buffer);
}

std::size_t ref_buffer::serialize_buffer(const input_buffer_ptr& ref,
	output_buffer_interface& buffer, std::size_t offset, std::size_t size,
	bool write_virtual_data) const
{
	auto ref_size = ref->size();
	if (!write_virtual_data)
		ref_size -= ref->virtual_size();

	if (size == npos)
	{
		size = offset > ref_size ? 0u : ref_size - offset;
	}
	else
	{
		if (!utilities::math::is_sum_safe(size, offset))
			throw std::system_error(utilities::generic_errc::integer_overflow);
	}

	if (size)
	{
		if (size + offset > ref_size)
			throw std::system_error(utilities::generic_errc::buffer_overrun);

		input_buffer_stateful_wrapper_ref wrapper(*ref);
		wrapper.set_rpos(offset);
		copy(wrapper, buffer, size);
	}
	return size;
}

std::size_t ref_buffer::serialize_until(output_buffer_interface& buffer,
	std::size_t offset, std::size_t size,
	bool write_virtual_data) const
{
	return std::visit(
		[this, &buffer, offset, size, write_virtual_data] (const auto& ref) {
			return serialize_buffer(ref.buffer, buffer, offset, size,
				write_virtual_data);
		}, buffer_
	);
}

void ref_buffer::serialize(output_buffer_interface& buffer,
	bool write_virtual_data) const
{
	std::visit(
		[this, &buffer, write_virtual_data](const auto& buf) {
			auto size = buf.buffer->size();
			if (!write_virtual_data)
				size -= buf.buffer->virtual_size();
			if (!size)
				return;

			buffers::input_buffer_stateful_wrapper_ref wrapper(*buf.buffer);
			copy(wrapper, buffer, size);
		}, buffer_
	);
}

void ref_buffer::read_buffer(input_buffer_interface& buf)
{
	auto physical_size = buf.physical_size();
	auto copied_buf = std::make_shared<input_container_buffer>(
		buf.absolute_offset(), buf.relative_offset());

	if (physical_size)
	{
		auto& container = copied_buf->get_container();
		container.resize(physical_size);
		auto read_bytes = buf.read(0u, container.size(), container.data());
		if (read_bytes != container.size())
			throw std::system_error(ref_buffer_errc::unable_to_read_data);
	}

	auto buf_ptr = copied_buf.get();
	if (buf.virtual_size())
	{
		auto virtual_buf = std::make_shared<input_virtual_buffer>(
			std::move(copied_buf), buf.virtual_size());
		buffer_ = copied_buffer(buf_ptr, std::move(virtual_buf));
	}
	else
	{
		buffer_ = copied_buffer(buf_ptr, std::move(copied_buf));
	}
}

bool ref_buffer::is_copied() const noexcept
{
	return buffer_.index() == 0u;
}

void ref_buffer::copy_referenced_buffer()
{
	if (auto ptr = std::get_if<buffer_ref>(&buffer_); ptr)
		read_buffer(*ptr->buffer);
}

input_buffer_ptr ref_buffer::data() const
{
	return std::visit([this](const auto& buf)
		-> input_buffer_ptr { return buf.buffer; }, buffer_);
}

ref_buffer::container_type& ref_buffer::copied_data()
{
	copy_referenced_buffer();
	return std::get<copied_buffer>(buffer_).container->get_container();
}

const ref_buffer::container_type& ref_buffer::copied_data() const
{
	if (const auto* buf = std::get_if<copied_buffer>(&buffer_); buf)
		return buf->container->get_container();
	else
		throw std::system_error(ref_buffer_errc::data_is_not_copied);
}

std::size_t ref_buffer::size() const
{
	return std::visit([this] (const auto& buf) { return buf.buffer->size(); }, buffer_);
}

std::size_t ref_buffer::virtual_size() const noexcept
{
	return std::visit([this](const auto& buf) { return buf.buffer->virtual_size(); }, buffer_);
}

bool ref_buffer::is_stateless() const noexcept
{
	return std::visit([this](const auto& buf) { return buf.buffer->is_stateless(); }, buffer_);
}

std::size_t ref_buffer::physical_size() const
{
	return size() - virtual_size();
}

} //namespace buffers
