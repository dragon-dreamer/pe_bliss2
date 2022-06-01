#include "pe_bliss2/image/image_section_search.h"

#include <iterator>

#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/pe_types.h"

namespace
{

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

template<typename Result, typename Reference,
	typename SectionTable, typename SectionDataList>
Result section_from_reference_impl(Reference& section_hdr,
	SectionTable& section_tbl, SectionDataList& section_data_list) noexcept
{
	return get_section_iterators<Result>(section_tbl.by_reference(section_hdr),
		section_tbl, section_data_list);
}

template<typename Result, typename Image, typename Va>
Result section_from_va_impl(Image& instance, Va va, std::uint32_t data_size)
{
	return section_from_rva_impl<Result>(
		pe_bliss::address_converter(instance).va_to_rva(va), data_size,
		instance.get_section_table(), instance.get_section_data_list(),
		instance.get_optional_header().get_raw_section_alignment());
}

template<typename Result, typename Image>
Result section_from_directory_impl(Image& instance,
	pe_bliss::core::data_directories::directory_type directory)
{
	auto& section_table = instance.get_section_table();
	auto& section_list = instance.get_section_data_list();
	const auto& data_directories = instance.get_data_directories();

	if (!data_directories.has_directory(directory))
		return { std::end(section_table.get_section_headers()), std::end(section_list) };

	return section_from_rva_impl<Result>(
		data_directories.get_directory(directory)->virtual_address, 0,
		section_table, section_list,
		instance.get_optional_header().get_raw_section_alignment());
}

} //namespace

namespace pe_bliss::image
{

section_ref section_from_reference(image& instance,
	section::section_header& section_hdr) noexcept
{
	return section_from_reference_impl<section_ref>(section_hdr,
		instance.get_section_table(), instance.get_section_data_list());
}

section_const_ref section_from_reference(const image& instance,
	const section::section_header& section_hdr) noexcept
{
	return section_from_reference_impl<section_const_ref>(section_hdr,
		instance.get_section_table(), instance.get_section_data_list());
}

section_ref section_from_rva(image& instance, rva_type rva,
	std::uint32_t data_size)
{
	return section_from_rva_impl<section_ref>(rva, data_size,
		instance.get_section_table(), instance.get_section_data_list(),
		instance.get_optional_header().get_raw_section_alignment());
}

section_const_ref section_from_rva(
	const image& instance, rva_type rva,
	std::uint32_t data_size)
{
	return section_from_rva_impl<section_const_ref>(rva, data_size,
		instance.get_section_table(), instance.get_section_data_list(),
		instance.get_optional_header().get_raw_section_alignment());
}

section_ref section_from_va(image& instance, std::uint32_t va,
	std::uint32_t data_size)
{
	return section_from_va_impl<section_ref>(instance, va, data_size);
}

section_const_ref section_from_va(
	const image& instance, std::uint32_t va,
	std::uint32_t data_size)
{
	return section_from_va_impl<section_const_ref>(instance, va, data_size);
}

section_ref section_from_va(image& instance, std::uint64_t va,
	std::uint32_t data_size)
{
	return section_from_va_impl<section_ref>(instance, va, data_size);
}

section_const_ref section_from_va(
	const image& instance, std::uint64_t va,
	std::uint32_t data_size)
{
	return section_from_va_impl<section_const_ref>(instance, va, data_size);
}

section_ref section_from_directory(image& instance,
	core::data_directories::directory_type directory)
{
	return section_from_directory_impl<section_ref>(instance, directory);
}

section_const_ref section_from_directory(const image& instance,
	core::data_directories::directory_type directory)
{
	return section_from_directory_impl<section_const_ref>(instance, directory);
}

section_ref section_from_file_offset(
	image& instance, std::uint32_t offset,
	std::uint32_t data_size)
{
	return section_from_raw_offset_impl<section_ref>(offset, data_size,
		instance.get_section_table(), instance.get_section_data_list(),
		instance.get_optional_header().get_raw_section_alignment());
}

section_const_ref section_from_file_offset(
	const image& instance, std::uint32_t offset,
	std::uint32_t data_size)
{
	return section_from_raw_offset_impl<section_const_ref>(offset, data_size,
		instance.get_section_table(), instance.get_section_data_list(),
		instance.get_optional_header().get_raw_section_alignment());
}

} //namespace pe_bliss::image
