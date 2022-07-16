#include "buffers/ref_buffer.h"

#include <cassert>
#include <memory>
#include <utility>
#include <variant>
#include <system_error>

#include "buffers/buffer_copy.h"
#include "buffers/input_buffer_interface.h"
#include "buffers/input_buffer_interface.h"
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
	: buffer_(std::in_place_type<copied_buffer>,
		std::make_shared<input_container_buffer>())
{
}

ref_buffer::ref_buffer(const ref_buffer& other)
{
	*this = other;
}

ref_buffer& ref_buffer::operator=(const ref_buffer& other)
{
	if (const auto* buf = std::get_if<copied_buffer>(&other.buffer_); buf)
	{
		auto buf_copy = std::make_shared<input_container_buffer>(
			buf->buffer->absolute_offset(), buf->buffer->relative_offset());
		buf_copy->get_container() = buf->buffer->get_container();
		buffer_ = copied_buffer(buf_copy);
	}
	else
	{
		buffer_ = other.buffer_;
	}
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
	output_buffer_interface& buffer, std::size_t offset, std::size_t size) const
{
	if (size == npos)
	{
		size = offset > ref->size() ? 0u : ref->size() - offset;
	}
	else
	{
		if (!utilities::math::is_sum_safe(size, offset))
			throw std::system_error(utilities::generic_errc::integer_overflow);
	}

	if (size)
	{
		if (size > ref->size())
			throw std::system_error(utilities::generic_errc::buffer_overrun);

		ref->set_rpos(offset);
		copy(*ref, buffer, size);
	}
	return size;
}

std::size_t ref_buffer::serialize(output_buffer_interface& buffer,
	std::size_t offset, std::size_t size) const
{
	return std::visit(
		[this, &buffer, offset, size] (const auto& ref) {
			return serialize_buffer(ref.buffer, buffer, offset, size);
		}, buffer_
	);
}

void ref_buffer::serialize(output_buffer_interface& buffer) const
{
	std::visit(
		[this, &buffer](const auto& buf) {
			auto size = buf.buffer->size();
			if (!size)
				return;

			buf.buffer->set_rpos(0);
			copy(*buf.buffer, buffer, size);
		}, buffer_
	);
}

void ref_buffer::read_buffer(input_buffer_interface& buf)
{
	auto size = buf.size();
	auto copied_buf = std::make_shared<input_container_buffer>(
		buf.absolute_offset(), buf.relative_offset());
	auto& container = copied_buf->get_container();
	container.resize(size);

	if (size)
	{
		buf.set_rpos(0u);
		auto read_bytes = buf.read(container.size(), container.data());
		if (read_bytes != container.size())
			throw std::system_error(ref_buffer_errc::unable_to_read_data);
	}

	buffer_ = copied_buffer(std::move(copied_buf));
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
	return std::get<copied_buffer>(buffer_).buffer->get_container();
}

const ref_buffer::container_type& ref_buffer::copied_data() const
{
	if (const auto* buf = std::get_if<copied_buffer>(&buffer_); buf)
		return buf->buffer->get_container();
	else
		throw std::system_error(ref_buffer_errc::data_is_not_copied);
}

bool ref_buffer::empty() const noexcept
{
	return !size();
}

std::size_t ref_buffer::size() const noexcept
{
	return std::visit([this] (const auto& buf) { return buf.buffer->size(); }, buffer_);
}

} //namespace buffers
