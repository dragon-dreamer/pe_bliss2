#include "pe_bliss2/file_header.h"

#include <cstdint>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/detail/image_data_directory.h"
#include "pe_bliss2/detail/packed_serialization.h"
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
		switch (static_cast<pe_bliss::file_header_errc>(ev))
		{
		case pe_bliss::file_header_errc::invalid_size_of_optional_header:
			return "Invalid size of optional header";
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

pe_error_wrapper file_header::validate_size_of_optional_header(
	const optional_header& hdr) const noexcept
{
	if (!base_struct()->number_of_sections && !base_struct()->size_of_optional_header)
		return {};

	constexpr auto data_dir_size = detail::packed_serialization<>
		::get_type_size<detail::image_data_directory>();
	if (base_struct()->size_of_optional_header
		< hdr.get_size_of_structure() + data_dir_size * hdr.get_number_of_rva_and_sizes())
	{
		return file_header_errc::invalid_size_of_optional_header;
	}

	return {};
}

} //namespace pe_bliss
