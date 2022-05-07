#include "pe_bliss2/file_header.h"

#include <cstdint>
#include <exception>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/detail/image_data_directory.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/optional_header.h"
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
		using enum pe_bliss::file_header_errc;
		switch (static_cast<pe_bliss::file_header_errc>(ev))
		{
		case invalid_size_of_optional_header:
			return "Invalid size of optional header";
		case unable_to_read_file_header:
			return "Unable to read file header";
		default:
			return {};
		}
	}
};

const file_header_error_category file_header_error_category_instance;

} //namespace

namespace pe_bliss
{

std::error_code make_error_code(file_header_errc e) noexcept
{
	return { static_cast<int>(e), file_header_error_category_instance };
}

std::size_t file_header::get_section_table_buffer_pos() const noexcept
{
	static constexpr auto file_header_size
		= detail::packed_reflection::get_type_size<detail::image_file_header>();
	return base_struct()->size_of_optional_header + file_header_size
		+ base_struct().get_state().buffer_pos();
}

void file_header::deserialize(buffers::input_buffer_interface& buf,
	bool allow_virtual_memory)
{
	try
	{
		base_struct().deserialize(buf, allow_virtual_memory);
	}
	catch (...)
	{
		std::throw_with_nested(pe_error(
			file_header_errc::unable_to_read_file_header));
	}
}

void file_header::serialize(buffers::output_buffer_interface& buf,
	bool write_virtual_part) const
{
	base_struct().serialize(buf, write_virtual_part);
}

pe_error_wrapper file_header::validate_size_of_optional_header(
	const optional_header& hdr) const noexcept
{
	if (!base_struct()->number_of_sections
		&& !base_struct()->size_of_optional_header)
	{
		return {};
	}

	constexpr auto data_dir_size = detail::packed_reflection
		::get_type_size<detail::image_data_directory>();
	if (base_struct()->size_of_optional_header
		< hdr.get_size_of_structure()
		+ data_dir_size * hdr.get_number_of_rva_and_sizes())
	{
		return file_header_errc::invalid_size_of_optional_header;
	}

	return {};
}

} //namespace pe_bliss
