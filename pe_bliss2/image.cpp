#include "pe_bliss2/image.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <system_error>

#include "buffers/input_buffer_section.h"
#include "buffers/output_memory_ref_buffer.h"
#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/detail/image_file_header.h"
#include "pe_bliss2/packed_c_string.h"
#include "pe_bliss2/packed_utf16_string.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/generic_error.h"
#include "utilities/math.h"

namespace
{

struct image_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "image";
	}

	std::string message(int ev) const override
	{
		switch (static_cast<pe_bliss::image_errc>(ev))
		{
		case pe_bliss::image_errc::too_many_sections:
			return "Too many sections";
		case pe_bliss::image_errc::too_many_rva_and_sizes:
			return "Too many data directories";
		case pe_bliss::image_errc::section_data_does_not_exist:
			return "Requested section data does not exist";
		default:
			return {};
		}
	}
};

const image_error_category image_error_category_instance;

template<typename Result, typename SectionHeaderIt,
	typename SectionTable, typename SectionDataList>
Result get_section_iterators(SectionHeaderIt header_it, SectionTable& section_tbl,
	SectionDataList& section_data_list) noexcept
{
	if (header_it == std::end(section_tbl.get_section_headers()))
		return { header_it, std::end(section_data_list) };

	auto header_index = std::distance(
		std::begin(section_tbl.get_section_headers()), header_it);
	return {
		header_it,
		std::next(std::begin(section_data_list), header_index)
	};
}

template<typename Result, typename SectionTable, typename SectionDataList>
Result section_from_rva_impl(pe_bliss::rva_type rva, std::uint32_t data_size,
	SectionTable& section_tbl, SectionDataList& section_data_list,
	std::uint32_t section_alignment)
{
	return get_section_iterators<Result>(
		section_tbl.by_rva(rva, section_alignment, data_size),
		section_tbl, section_data_list);
}

template<typename Result, typename SectionTable, typename SectionDataList>
Result section_from_raw_offset_impl(std::uint32_t raw_offset, std::uint32_t data_size,
	SectionTable& section_tbl, SectionDataList& section_data_list,
	std::uint32_t section_alignment)
{
	return get_section_iterators<Result>(
		section_tbl.by_raw_offset(raw_offset, section_alignment, data_size),
		section_tbl, section_data_list);
}

template<typename Result, typename Reference, typename SectionTable, typename SectionDataList>
Result section_from_reference_impl(Reference& section_hdr,
	SectionTable& section_tbl, SectionDataList& section_data_list) noexcept
{
	return get_section_iterators<Result>(section_tbl.by_reference(section_hdr),
		section_tbl, section_data_list);
}

} //namespace

namespace pe_bliss
{

std::error_code make_error_code(image_errc e) noexcept
{
	return { static_cast<int>(e), image_error_category_instance };
}

bool image::has_relocation() const noexcept
{
	return data_directories_.has_reloc()
		&& !(file_header_.get_characteristics() & file_header::characteristics::relocs_stripped);
}

bool image::is_64bit() const noexcept
{
	return optional_header_.get_magic() == optional_header::magic::pe64;
}

void image::update_number_of_sections()
{
	auto number_of_sections = section_table_.get_section_headers().size();
	using number_of_sections_type = decltype(detail::image_file_header::number_of_sections);
	if (number_of_sections > (std::numeric_limits<number_of_sections_type>::max)())
		throw pe_error(image_errc::too_many_sections);

	file_header_.base_struct()->number_of_sections
		= static_cast<number_of_sections_type>(number_of_sections);
}

void image::set_number_of_data_directories(std::uint32_t number)
{
	if (number > optional_header::max_number_of_rva_and_sizes)
		throw pe_error(image_errc::too_many_rva_and_sizes);

	data_directories_.set_size(number);
	optional_header_.set_raw_number_of_rva_and_sizes(number);
}

image::section_ref image::section_from_rva(rva_type rva,
	std::uint32_t data_size)
{
	return section_from_rva_impl<image::section_ref>(rva, data_size,
		section_table_, section_list_, optional_header_.get_raw_section_alignment());
}

image::section_const_ref image::section_from_rva(rva_type rva,
	std::uint32_t data_size) const
{
	return section_from_rva_impl<image::section_const_ref>(rva, data_size,
		section_table_, section_list_, optional_header_.get_raw_section_alignment());
}

image::section_ref image::section_from_va(std::uint32_t va,
	std::uint32_t data_size)
{
	return section_from_rva_impl<image::section_ref>(
		address_converter(*this).va_to_rva(va), data_size,
		section_table_, section_list_, optional_header_.get_raw_section_alignment());
}

image::section_const_ref image::section_from_va(std::uint32_t va,
	std::uint32_t data_size) const
{
	return section_from_rva_impl<image::section_const_ref>(
		address_converter(*this).va_to_rva(va), data_size,
		section_table_, section_list_, optional_header_.get_raw_section_alignment());
}

image::section_ref image::section_from_va(std::uint64_t va,
	std::uint32_t data_size)
{
	return section_from_rva_impl<image::section_ref>(
		address_converter(*this).va_to_rva(va), data_size,
		section_table_, section_list_, optional_header_.get_raw_section_alignment());
}

image::section_const_ref image::section_from_va(std::uint64_t va,
	std::uint32_t data_size) const
{
	return section_from_rva_impl<image::section_const_ref>(
		address_converter(*this).va_to_rva(va), data_size,
		section_table_, section_list_, optional_header_.get_raw_section_alignment());
}

image::section_ref image::section_from_directory(
	data_directories::directory_type directory)
{
	if (!data_directories_.has_directory(directory))
		return { std::end(section_table_.get_section_headers()), std::end(section_list_) };

	return section_from_rva_impl<image::section_ref>(
		data_directories_.get_directory(directory)->virtual_address, 0,
		section_table_, section_list_, optional_header_.get_raw_section_alignment());
}

image::section_const_ref image::section_from_directory(
	data_directories::directory_type directory) const
{
	if (!data_directories_.has_directory(directory))
		return { std::cend(section_table_.get_section_headers()), std::cend(section_list_) };

	return section_from_rva_impl<image::section_const_ref>(
		data_directories_.get_directory(directory)->virtual_address, 0,
		section_table_, section_list_, optional_header_.get_raw_section_alignment());
}

image::section_ref image::section_from_file_offset(std::uint32_t offset,
	std::uint32_t data_size)
{
	return section_from_raw_offset_impl<image::section_ref>(offset, data_size,
		section_table_, section_list_, optional_header_.get_raw_section_alignment());
}

image::section_const_ref image::section_from_file_offset(std::uint32_t offset,
	std::uint32_t data_size) const
{
	return section_from_raw_offset_impl<image::section_const_ref>(offset, data_size,
		section_table_, section_list_, optional_header_.get_raw_section_alignment());
}

void image::update_image_size()
{
	if (section_table_.get_section_headers().empty())
	{
		optional_header_.set_raw_size_of_image(
			optional_header_.get_raw_size_of_headers());
	}
	else
	{
		const auto& last_section = section_table_.get_section_headers().back();
		auto size_of_image = last_section.get_rva();
		if (!utilities::math::add_if_safe(size_of_image,
			last_section.get_virtual_size(optional_header_.get_raw_section_alignment())))
		{
			throw pe_error(utilities::generic_errc::integer_overflow);
		}
		optional_header_.set_raw_size_of_image(size_of_image);
	}
}

std::uint32_t image::strip_data_directories(std::uint32_t min_count)
{
	auto result = data_directories_.strip_data_directories(min_count);
	optional_header_.set_raw_number_of_rva_and_sizes(result);
	return result;
}

void image::copy_referenced_section_memory()
{
	for (auto& section : section_list_)
		section.copy_referenced_buffer();
}

std::uint32_t image::file_offset_to_rva(std::uint32_t offset) const
{
	if (offset < optional_header_.get_raw_size_of_headers())
		return offset;

	auto it = section_from_file_offset(offset, 1u).first;
	std::uint32_t result = offset - it->get_pointer_to_raw_data();
	if (!utilities::math::add_if_safe(result, it->get_rva()))
		throw pe_error(utilities::generic_errc::integer_overflow);

	return result;
}

std::uint32_t image::rva_to_file_offset(rva_type rva) const
{
	if (rva < optional_header_.get_raw_size_of_headers())
		return rva;

	auto it = section_from_rva(rva, 1u).first;
	std::uint32_t result = rva - it->get_rva();
	if (!utilities::math::add_if_safe(result, it->get_pointer_to_raw_data()))
		throw pe_error(utilities::generic_errc::integer_overflow);

	return result;
}

buffers::input_buffer_ptr image::section_data_from_rva(rva_type rva,
	std::uint32_t data_size, bool include_headers, bool allow_virtual_data) const
{
	if (!utilities::math::is_sum_safe(rva, data_size))
		throw pe_error(utilities::generic_errc::integer_overflow);

	if (include_headers && (rva + data_size <= full_headers_buffer_.size()))
		return buffers::reduce(full_headers_buffer_.data(), rva, data_size);

	auto [header_it, data_it] = section_from_rva(rva, data_size);
	if (data_it == std::cend(section_list_))
		throw pe_error(image_errc::section_data_does_not_exist);

	std::uint32_t data_offset = rva - header_it->get_rva();
	if (data_offset + data_size <= data_it->size())
		return buffers::reduce(data_it->data(), data_offset, data_size);

	if (allow_virtual_data)
	{
		data_size = data_offset < data_it->size()
			? static_cast<std::uint32_t>(data_it->size()) - data_offset
			: 0u;
		return buffers::reduce(data_it->data(), (std::min)(data_it->size(),
			static_cast<std::size_t>(data_offset)), data_size);
	}

	throw pe_error(image_errc::section_data_does_not_exist);
}

std::span<std::byte> image::section_data_from_rva(rva_type rva,
	std::uint32_t data_size, bool include_headers, bool allow_virtual_data)
{
	if (!utilities::math::is_sum_safe(rva, data_size))
		throw pe_error(utilities::generic_errc::integer_overflow);

	if (include_headers && (rva + data_size <= full_headers_buffer_.size()))
		return { full_headers_buffer_.copied_data().data() + rva, data_size };

	auto [header_it, data_it] = section_from_rva(rva, data_size);
	if (data_it == std::end(section_list_))
		throw pe_error(image_errc::section_data_does_not_exist);

	std::uint32_t data_offset = rva - header_it->get_rva();
	if (data_offset + data_size <= data_it->size())
		return { data_it->copied_data().data() + data_offset, data_size };

	if (allow_virtual_data)
	{
		data_size = data_offset < data_it->size()
			? static_cast<std::uint32_t>(data_it->size()) - data_offset
			: 0u;
		return { data_it->copied_data().data()
			+ (std::min)(data_it->size(), static_cast<std::size_t>(data_offset)), data_size };
	}

	throw pe_error(image_errc::section_data_does_not_exist);
}

buffers::input_buffer_ptr image::section_data_from_va(std::uint32_t va,
	std::uint32_t data_size, bool include_headers, bool allow_virtual_data) const
{
	return section_data_from_rva(address_converter(*this).va_to_rva(va),
		data_size, include_headers, allow_virtual_data);
}

std::span<std::byte> image::section_data_from_va(std::uint32_t va,
	std::uint32_t data_size, bool include_headers, bool allow_virtual_data)
{
	return section_data_from_rva(address_converter(*this).va_to_rva(va),
		data_size, include_headers, allow_virtual_data);
}

buffers::input_buffer_ptr image::section_data_from_va(std::uint64_t va,
	std::uint32_t data_size, bool include_headers, bool allow_virtual_data) const
{
	return section_data_from_rva(address_converter(*this).va_to_rva(va),
		data_size, include_headers, allow_virtual_data);
}

std::span<std::byte> image::section_data_from_va(std::uint64_t va,
	std::uint32_t data_size, bool include_headers, bool allow_virtual_data)
{
	return section_data_from_rva(address_converter(*this).va_to_rva(va),
		data_size, include_headers, allow_virtual_data);
}

std::uint32_t image::section_data_length_from_rva(rva_type rva,
	bool include_headers, bool allow_virtual_data) const
{
	if (include_headers && rva <= full_headers_buffer_.size())
		return static_cast<std::uint32_t>(full_headers_buffer_.size()) - rva;

	auto [header_it, data_it] = section_from_rva(rva, 1u);
	if (data_it == std::end(section_list_))
		throw pe_error(image_errc::section_data_does_not_exist);

	std::uint32_t data_offset = rva - header_it->get_rva();
	if (allow_virtual_data)
	{
		return header_it->get_virtual_size(optional_header_.get_raw_section_alignment())
			- data_offset;
	}

	if (data_offset >= data_it->size())
		return 0u;

	return static_cast<std::uint32_t>(data_it->size()) - data_offset;
}

std::uint32_t image::section_data_length_from_va(std::uint32_t va,
	bool include_headers, bool allow_virtual_data) const
{
	return section_data_length_from_rva(address_converter(*this).va_to_rva(va),
		include_headers, allow_virtual_data);
}

std::uint32_t image::section_data_length_from_va(std::uint64_t va,
	bool include_headers, bool allow_virtual_data) const
{
	return section_data_length_from_rva(address_converter(*this).va_to_rva(va),
		include_headers, allow_virtual_data);
}

image::section_ref image::section_from_reference(
	section_header& section_hdr) noexcept
{
	return section_from_reference_impl<section_ref>(section_hdr,
		get_section_table(), get_section_data_list());
}

image::section_const_ref image::section_from_reference(
	const section_header& section_hdr) const noexcept
{
	return section_from_reference_impl<section_const_ref>(section_hdr,
		get_section_table(), get_section_data_list());
}

template<detail::packed_string_type PackedString>
PackedString image::string_from_rva(rva_type rva,
	bool include_headers, bool allow_virtual_data) const
{
	PackedString result;
	string_from_rva(rva, result, include_headers, allow_virtual_data);
	return result;
}

template<detail::packed_string_type PackedString>
void image::string_from_rva(rva_type rva, PackedString& str,
	bool include_headers, bool allow_virtual_data) const
{
	auto buf = section_data_from_rva(rva, include_headers);
	str.deserialize(*buf, allow_virtual_data);
}

template<detail::packed_string_type PackedString>
PackedString image::string_from_va(std::uint32_t va,
	bool include_headers, bool allow_virtual_data) const
{
	return string_from_rva<PackedString>(
		address_converter(*this).va_to_rva(va),
		include_headers, allow_virtual_data);
}

template<detail::packed_string_type PackedString>
void image::string_from_va(std::uint32_t va, PackedString& str,
	bool include_headers, bool allow_virtual_data) const
{
	string_from_rva(address_converter(*this).va_to_rva(va), str,
		include_headers, allow_virtual_data);
}

template<detail::packed_string_type PackedString>
PackedString image::string_from_va(std::uint64_t va,
	bool include_headers, bool allow_virtual_data) const
{
	return string_from_rva<PackedString>(
		address_converter(*this).va_to_rva(va),
		include_headers, allow_virtual_data);
}

template<detail::packed_string_type PackedString>
void image::string_from_va(std::uint64_t va, PackedString& str,
	bool include_headers, bool allow_virtual_data) const
{
	string_from_rva(address_converter(*this).va_to_rva(va), str,
		include_headers, allow_virtual_data);
}

template packed_c_string image::string_from_rva<
	packed_c_string>(rva_type rva,
	bool include_headers, bool allow_virtual_data) const;
template void image::string_from_rva<
	packed_c_string>(rva_type rva, packed_c_string& str,
	bool include_headers, bool allow_virtual_data) const;
template packed_c_string image::string_from_va<
	packed_c_string>(std::uint32_t va,
	bool include_headers, bool allow_virtual_data) const;
template void image::string_from_va<
	packed_c_string>(std::uint32_t va, packed_c_string& str,
	bool include_headers, bool allow_virtual_data) const;
template packed_c_string image::string_from_va<
	packed_c_string>(std::uint64_t va,
	bool include_headers, bool allow_virtual_data) const;
template void image::string_from_va<
	packed_c_string>(std::uint64_t va, packed_c_string& str,
	bool include_headers, bool allow_virtual_data) const;

template packed_utf16_string image::string_from_rva<
	packed_utf16_string>(rva_type rva,
		bool include_headers, bool allow_virtual_data) const;
template void image::string_from_rva<
	packed_utf16_string>(rva_type rva, packed_utf16_string& str,
		bool include_headers, bool allow_virtual_data) const;
template packed_utf16_string image::string_from_va<
	packed_utf16_string>(std::uint32_t va,
		bool include_headers, bool allow_virtual_data) const;
template void image::string_from_va<
	packed_utf16_string>(std::uint32_t va, packed_utf16_string& str,
		bool include_headers, bool allow_virtual_data) const;
template packed_utf16_string image::string_from_va<
	packed_utf16_string>(std::uint64_t va,
		bool include_headers, bool allow_virtual_data) const;
template void image::string_from_va<
	packed_utf16_string>(std::uint64_t va, packed_utf16_string& str,
		bool include_headers, bool allow_virtual_data) const;

buffers::input_buffer_ptr image::section_data_from_rva(rva_type rva,
	bool include_headers) const
{
	if (include_headers && rva < full_headers_buffer_.size())
		return buffers::reduce(full_headers_buffer_.data(), rva, full_headers_buffer_.size() - rva);

	auto [header_it, data_it] = section_from_rva(rva, 1u);
	if (data_it == std::cend(section_list_))
		throw pe_error(image_errc::section_data_does_not_exist);

	std::uint32_t data_offset = rva - header_it->get_rva();
	return buffers::reduce(data_it->data(), data_offset, data_it->size() - data_offset);
}

std::span<std::byte> image::section_data_from_rva(rva_type rva,
	bool include_headers)
{
	if (include_headers && rva < full_headers_buffer_.size())
	{
		return { full_headers_buffer_.copied_data().data() + rva,
			full_headers_buffer_.size() - rva };
	}

	auto [header_it, data_it] = section_from_rva(rva, 1u);
	if (data_it == std::end(section_list_))
		throw pe_error(image_errc::section_data_does_not_exist);

	std::uint32_t data_offset = rva - header_it->get_rva();
	return { data_it->copied_data().data() + data_offset, data_it->size() - data_offset };
}

buffers::input_buffer_ptr image::section_data_from_va(std::uint32_t va,
	bool include_headers) const
{
	return section_data_from_rva(address_converter(*this).va_to_rva(va), include_headers);
}

std::span<std::byte> image::section_data_from_va(std::uint32_t va,
	bool include_headers)
{
	return section_data_from_rva(address_converter(*this).va_to_rva(va), include_headers);
}

buffers::input_buffer_ptr image::section_data_from_va(std::uint64_t va,
	bool include_headers) const
{
	return section_data_from_rva(address_converter(*this).va_to_rva(va), include_headers);
}

std::span<std::byte> image::section_data_from_va(std::uint64_t va,
	bool include_headers)
{
	return section_data_from_rva(address_converter(*this).va_to_rva(va), include_headers);
}

template<detail::packed_string_type PackedString>
rva_type image::string_to_file_offset(const PackedString& str,
	bool include_headers, bool write_virtual_part)
{
	return string_to_rva(absolute_offset_to_rva(str), str, include_headers, write_virtual_part);
}

template<detail::packed_string_type PackedString>
rva_type image::string_to_rva(rva_type rva, const PackedString& str,
	bool include_headers, bool write_virtual_part)
{
	if (str.value().empty() && str.is_virtual() && !write_virtual_part)
		return rva;

	auto buf = section_data_from_rva(rva, include_headers);
	return rva + static_cast<rva_type>(
		str.serialize(buf.data(), buf.size_bytes(), write_virtual_part));
}

template<detail::packed_string_type PackedString>
std::uint32_t image::string_to_va(std::uint32_t va, const PackedString& str,
	bool include_headers, bool write_virtual_part)
{
	if (str.value().empty() && str.is_virtual() && !write_virtual_part)
		return va;

	auto buf = section_data_from_va(va, include_headers);
	return va + static_cast<std::uint32_t>(
		str.serialize(buf.data(), buf.size_bytes(), write_virtual_part));
}

template<detail::packed_string_type PackedString>
std::uint64_t image::string_to_va(std::uint64_t va, const PackedString& str,
	bool include_headers, bool write_virtual_part)
{
	if (str.value().empty() && str.is_virtual() && !write_virtual_part)
		return va;

	auto buf = section_data_from_va(va, include_headers);
	return va + static_cast<std::uint64_t>(
		str.serialize(buf.data(), buf.size_bytes(), write_virtual_part));
}

template rva_type image::string_to_rva<packed_c_string>(
	rva_type rva, const packed_c_string& str,
	bool include_headers, bool write_virtual_part);
template std::uint32_t image::string_to_va<packed_c_string>(
	std::uint32_t va, const packed_c_string& str,
	bool include_headers, bool write_virtual_part);
template std::uint64_t image::string_to_va<packed_c_string>(
	std::uint64_t va, const packed_c_string& str,
	bool include_headers, bool write_virtual_part);
template rva_type image::string_to_file_offset<packed_c_string>(
	const packed_c_string& str,
	bool include_headers, bool write_virtual_part);

template rva_type image::string_to_rva<packed_utf16_string>(
	rva_type rva, const packed_utf16_string& str,
	bool include_headers, bool write_virtual_part);
template std::uint32_t image::string_to_va<packed_utf16_string>(
	std::uint32_t va, const packed_utf16_string& str,
	bool include_headers, bool write_virtual_part);
template std::uint64_t image::string_to_va<packed_utf16_string>(
	std::uint64_t va, const packed_utf16_string& str,
	bool include_headers, bool write_virtual_part);
template rva_type image::string_to_file_offset<packed_utf16_string>(
	const packed_utf16_string& str,
	bool include_headers, bool write_virtual_part);

rva_type image::buffer_to_rva(rva_type rva, const buffers::ref_buffer& buf,
	bool include_headers)
{
	if (buf.empty())
		return rva;

	auto data = section_data_from_rva(rva, include_headers);
	auto data_buffer = buffers::output_memory_ref_buffer(data);
	buf.serialize(data_buffer);
	return rva + static_cast<rva_type>(buf.size());
}

std::uint32_t image::buffer_to_va(std::uint32_t va, const buffers::ref_buffer& buf,
	bool include_headers)
{
	if (buf.empty())
		return va;

	auto data = section_data_from_va(va, include_headers);
	auto data_buffer = buffers::output_memory_ref_buffer(data);
	buf.serialize(data_buffer);
	return va + static_cast<std::uint32_t>(buf.size());
}

std::uint64_t image::buffer_to_va(std::uint64_t va, const buffers::ref_buffer& buf,
	bool include_headers)
{
	if (buf.empty())
		return va;

	auto data = section_data_from_va(va, include_headers);
	auto data_buffer = buffers::output_memory_ref_buffer(data);
	buf.serialize(data_buffer);
	return va + static_cast<std::uint64_t>(buf.size());
}

rva_type image::buffer_to_file_offset(const buffers::ref_buffer& buf,
	bool include_headers)
{
	return buffer_to_rva(absolute_offset_to_rva(*buf.data()), buf, include_headers);
}

packed_byte_vector image::byte_vector_from_rva(rva_type rva,
	std::uint32_t size, bool include_headers, bool allow_virtual_data) const
{
	packed_byte_vector result;
	byte_vector_from_rva(rva, result, size, include_headers, allow_virtual_data);
	return result;
}

void image::byte_vector_from_rva(rva_type rva, packed_byte_vector& arr,
	std::uint32_t size, bool include_headers, bool allow_virtual_data) const
{
	auto buf = section_data_from_rva(rva, size, include_headers, allow_virtual_data);
	arr.deserialize(*buf, size, allow_virtual_data);
}

rva_type image::absolute_offset_to_rva(const buffers::input_buffer_interface& buf) const
{
	utilities::safe_uint<std::uint32_t> offset;
	offset += buf.absolute_offset();
	return file_offset_to_rva(offset.value());
}

/*
//TODO
//Prepares section before attaching it
void pe_base::prepare_section(section& s)
{
	//Calculate its size of raw data
	s.set_size_of_raw_data(static_cast<uint32_t>(pe_utils::align_up(s.get_raw_data().length(), get_file_alignment())));

	//Check section virtual and raw size
	if(!s.get_size_of_raw_data() && !s.get_virtual_size())
		throw pe_exception("Virtual and Physical sizes of section can't be 0 at the same time", pe_exception::zero_section_sizes);

	//If section virtual size is zero
	if(!s.get_virtual_size())
	{
		s.set_virtual_size(s.get_size_of_raw_data());
	}
	else
	{
		//Else calculate its virtual size
		s.set_virtual_size(
			std::max<uint32_t>(pe_utils::align_up(s.get_size_of_raw_data(), get_file_alignment()),
			pe_utils::align_up(s.get_virtual_size(), get_section_alignment())));
	}
}

//Adds section to image
section& pe_base::add_section(section s)
{
	if(sections_.size() >= maximum_number_of_sections)
		throw pe_exception("Maximum number of sections has been reached", pe_exception::no_more_sections_can_be_added);

	//Prepare section before adding it
	prepare_section(s);

	//Calculate section virtual address
	if(!sections_.empty())
	{
		s.set_virtual_address(pe_utils::align_up(sections_.back().get_virtual_address() + sections_.back().get_aligned_virtual_size(get_section_alignment()), get_section_alignment()));

		//We should align last section raw size, if it wasn't aligned
		section& last = sections_.back();
		last.set_size_of_raw_data(static_cast<uint32_t>(pe_utils::align_up(last.get_raw_data().length(), get_file_alignment())));
	}
	else
	{
		s.set_virtual_address(
			s.get_virtual_address() == 0
			? pe_utils::align_up(get_size_of_headers(), get_section_alignment())
			: pe_utils::align_up(s.get_virtual_address(), get_section_alignment()));
	}

	//Add section to the end of section list
	sections_.push_back(s);
	//Set number of sections in PE header
	set_number_of_sections(static_cast<uint16_t>(sections_.size()));
	//Recalculate virtual size of image
	set_size_of_image(get_size_of_image() + s.get_aligned_virtual_size(get_section_alignment()));
	//Return last section
	return sections_.back();
}

//Helper function to recalculate RAW and virtual section sizes and strip it, if necessary
void pe_base::recalculate_section_sizes(section& s, bool auto_strip)
{
	prepare_section(s); //Recalculate section raw addresses

	//Strip RAW size of section, if it is the last one
	//For all others it must be file-aligned and calculated by prepare_section() call
	if(auto_strip && !(sections_.empty() || &s == &*(sections_.end() - 1)))
	{
		//Strip ending raw data nullbytes to optimize size
		std::string& raw_data = s.get_raw_data();
		if(!raw_data.empty())
		{
			std::string::size_type i = raw_data.length();
			for(; i != 1; --i)
			{
				if(raw_data[i - 1] != 0)
					break;
			}

			raw_data.resize(i);
		}

		s.set_size_of_raw_data(static_cast<uint32_t>(raw_data.length()));
	}

	//Can occur only for last section
	if(pe_utils::align_up(s.get_virtual_size(), get_section_alignment()) < pe_utils::align_up(s.get_size_of_raw_data(), get_file_alignment()))
		set_section_virtual_size(s, pe_utils::align_up(s.get_size_of_raw_data(), get_section_alignment())); //Recalculate section virtual size
}

//Realigns section by index
void pe_base::realign_section(uint32_t index)
{
	//Check index
	if(sections_.size() <= index)
		throw pe_exception("Section not found", pe_exception::section_not_found);

	//Get section iterator
	section_list::iterator it = sections_.begin() + index;
	section& s = *it;

	//Calculate, how many null bytes we have in the end of raw section data
	std::size_t strip = 0;
	for(std::size_t i = (*it).get_raw_data().length(); i >= 1; --i)
	{
		if(s.get_raw_data()[i - 1] == 0)
			strip++;
		else
			break;
	}

	if(it == sections_.end() - 1) //If we're realigning the last section
	{
		//We can strip ending null bytes
		s.set_size_of_raw_data(static_cast<uint32_t>(s.get_raw_data().length() - strip));
		s.get_raw_data().resize(s.get_raw_data().length() - strip, 0);
	}
	else
	{
		//Else just set size of raw data
		uint32_t raw_size_aligned = s.get_aligned_raw_size(get_file_alignment());
		s.set_size_of_raw_data(raw_size_aligned);
		s.get_raw_data().resize(raw_size_aligned, 0);
	}
}
*/

} //namespace pe_bliss
