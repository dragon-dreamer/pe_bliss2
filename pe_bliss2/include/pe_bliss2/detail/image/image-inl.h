#pragma once

namespace pe_bliss::image
{

dos::dos_header& image::get_dos_header() & noexcept
{
	return dos_header_;
}

const dos::dos_header& image::get_dos_header() const& noexcept
{
	return dos_header_;
}

dos::dos_header image::get_dos_header() && noexcept
{
	return std::move(dos_header_);
}

dos::dos_stub& image::get_dos_stub() & noexcept
{
	return dos_stub_;
}

const dos::dos_stub& image::get_dos_stub() const& noexcept
{
	return dos_stub_;
}

dos::dos_stub image::get_dos_stub() && noexcept
{
	return std::move(dos_stub_);
}

core::image_signature& image::get_image_signature() & noexcept
{
	return image_signature_;
}

const core::image_signature& image::get_image_signature() const& noexcept
{
	return image_signature_;
}

core::image_signature image::get_image_signature() && noexcept
{
	return std::move(image_signature_);
}

core::file_header& image::get_file_header() & noexcept
{
	return file_header_;
}

const core::file_header& image::get_file_header() const& noexcept
{
	return file_header_;
}

core::file_header image::get_file_header() && noexcept
{
	return std::move(file_header_);
}

core::optional_header& image::get_optional_header() & noexcept
{
	return optional_header_;
}

const core::optional_header& image::get_optional_header() const& noexcept
{
	return optional_header_;
}

core::optional_header image::get_optional_header() && noexcept
{
	return std::move(optional_header_);
}

core::data_directories& image::get_data_directories() & noexcept
{
	return data_directories_;
}

const core::data_directories& image::get_data_directories() const& noexcept
{
	return data_directories_;
}

core::data_directories image::get_data_directories() && noexcept
{
	return std::move(data_directories_);
}

section::section_table& image::get_section_table() & noexcept
{
	return section_table_;
}

const section::section_table& image::get_section_table() const& noexcept
{
	return section_table_;
}

section::section_table image::get_section_table() && noexcept
{
	return std::move(section_table_);
}

section::section_data_list& image::get_section_data_list() & noexcept
{
	return section_list_;
}

const section::section_data_list& image::get_section_data_list() const& noexcept
{
	return section_list_;
}

section::section_data_list image::get_section_data_list() && noexcept
{
	return std::move(section_list_);
}

core::overlay& image::get_overlay() & noexcept
{
	return overlay_;
}

const core::overlay& image::get_overlay() const& noexcept
{
	return overlay_;
}

core::overlay image::get_overlay() && noexcept
{
	return std::move(overlay_);
}

buffers::ref_buffer& image::get_full_headers_buffer() & noexcept
{
	return full_headers_buffer_;
}

const buffers::ref_buffer& image::get_full_headers_buffer() const& noexcept
{
	return full_headers_buffer_;
}

buffers::ref_buffer image::get_full_headers_buffer() && noexcept
{
	return std::move(full_headers_buffer_);
}

buffers::ref_buffer& image::get_full_sections_buffer() & noexcept
{
	return full_sections_buffer_;
}

const buffers::ref_buffer& image::get_full_sections_buffer() const& noexcept
{
	return full_sections_buffer_;
}

buffers::ref_buffer image::get_full_sections_buffer() && noexcept
{
	return std::move(full_sections_buffer_);
}

} //namespace pe_bliss::image
