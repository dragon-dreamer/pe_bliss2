#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <system_error>
#include <type_traits>
#include <variant>

#include "buffers/input_buffer_interface.h"
#include "buffers/input_container_buffer.h"

namespace buffers
{

class output_buffer_interface;

enum class ref_buffer_errc
{
	unable_to_read_data = 1,
	data_is_not_copied
};

std::error_code make_error_code(ref_buffer_errc) noexcept;

class [[nodiscard]] ref_buffer
{
public:
	static constexpr auto npos = (std::numeric_limits<std::size_t>::max)();
	using container_type = input_container_buffer::container_type;

public:
	ref_buffer();
	ref_buffer(const ref_buffer&);
	ref_buffer(ref_buffer&&) = default;
	ref_buffer& operator=(const ref_buffer&);
	ref_buffer& operator=(ref_buffer&&) = default;

	void deserialize(const input_buffer_ptr& buffer, bool copy_memory);
	void serialize(output_buffer_interface& buffer,
		bool write_virtual_data = false) const;
	std::size_t serialize_until(output_buffer_interface& buffer,
		std::size_t offset, std::size_t size = npos,
		bool write_virtual_data = false) const;

	[[nodiscard]]
	input_buffer_ptr data() const;

	[[nodiscard]]
	container_type& copied_data();

	[[nodiscard]]
	const container_type& copied_data() const;

	void copy_referenced_buffer();

	[[nodiscard]] std::size_t size() const;
	[[nodiscard]] std::size_t virtual_size() const noexcept;
	[[nodiscard]] std::size_t physical_size() const;
	[[nodiscard]] bool is_stateless() const noexcept;

	[[nodiscard]] bool is_copied() const noexcept;

private:
	struct buffer_ref
	{
		input_buffer_ptr buffer;
	};

	struct copied_buffer
	{
		input_container_buffer* container;
		input_buffer_ptr buffer;
	};

private:
	void read_buffer(input_buffer_interface& buf);
	std::size_t serialize_buffer(const input_buffer_ptr& buf,
		output_buffer_interface& buffer,
		std::size_t offset, std::size_t size,
		bool write_virtual_data) const;

private:
	std::variant<copied_buffer, buffer_ref> buffer_;
};

} //namespace buffers

namespace std
{
template<>
struct is_error_code_enum<buffers::ref_buffer_errc> : true_type {};
} //namespace std
