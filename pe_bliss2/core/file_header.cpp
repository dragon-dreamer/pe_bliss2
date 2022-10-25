#include "pe_bliss2/core/file_header.h"

#include <cstdint>
#include <exception>
#include <system_error>
#include <type_traits>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/pe_error.h"

namespace
{

struct file_header_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "file_header";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::core::file_header_errc;
		switch (static_cast<pe_bliss::core::file_header_errc>(ev))
		{
		case unable_to_read_file_header:
			return "Unable to read file header";
		default:
			return {};
		}
	}
};

const file_header_error_category file_header_error_category_instance;

} //namespace

namespace pe_bliss::core
{

std::error_code make_error_code(file_header_errc e) noexcept
{
	return { static_cast<int>(e), file_header_error_category_instance };
}

std::size_t file_header::get_section_table_buffer_pos() const noexcept
{
	static constexpr auto file_header_size
		= detail::packed_reflection::get_type_size<detail::image_file_header>();
	return get_descriptor()->size_of_optional_header + file_header_size
		+ get_descriptor().get_state().buffer_pos();
}

void file_header::deserialize(buffers::input_buffer_stateful_wrapper_ref& buf,
	bool allow_virtual_memory)
{
	try
	{
		get_descriptor().deserialize(buf, allow_virtual_memory);
	}
	catch (const std::system_error&)
	{
		std::throw_with_nested(pe_error(
			file_header_errc::unable_to_read_file_header));
	}
}

void file_header::serialize(buffers::output_buffer_interface& buf,
	bool write_virtual_part) const
{
	get_descriptor().serialize(buf, write_virtual_part);
}

} //namespace pe_bliss::core
