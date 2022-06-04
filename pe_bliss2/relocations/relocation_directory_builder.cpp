#include "pe_bliss2/relocations/relocation_directory_builder.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "buffers/output_buffer_interface.h"
#include "buffers/output_memory_ref_buffer.h"
#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/detail/relocations/image_base_relocation.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/image/struct_to_va.h"
#include "utilities/generic_error.h"
#include "utilities/safe_uint.h"

namespace
{

using namespace pe_bliss;
using namespace pe_bliss::relocations;

template<typename BaseReloc>
std::size_t get_element_count(const BaseReloc& basereloc) noexcept
{
	auto elem_count = basereloc.get_relocations().size();
	for (const auto& entry : basereloc.get_relocations())
		elem_count += entry.get_param().has_value();
	return elem_count;
}

template<typename Directory>
std::uint32_t get_built_size_impl(const Directory& relocs, const builder_options& options) noexcept
{
	std::size_t descriptor_count = relocs.size();
	std::size_t total_elem_count = 0;
	for (const auto& basereloc : relocs)
	{
		auto elem_count = get_element_count(basereloc);
		if (options.align_base_relocation_structures && (elem_count % 2))
			++elem_count;

		total_elem_count += elem_count;
	}

	utilities::safe_uint<std::uint32_t> result;
	result += descriptor_count
		* Directory::value_type::packed_descriptor_type::packed_size;
	result += total_elem_count
		* Directory::value_type::entry_list_type::value_type::packed_descriptor_type::packed_size;
	return result.value();
}

void update_data_directory(image::image& instance, const builder_options& options, std::uint32_t size)
{
	if (options.update_data_directory)
	{
		auto& dir = instance.get_data_directories().get_directory(
			core::data_directories::directory_type::basereloc);
		dir->virtual_address = options.directory_rva;
		dir->size = size;
	}
}

template<typename Directory>
void build_in_place_impl(image::image& instance, const Directory& relocs,
	const builder_options& options)
{
	assert(options.directory_rva);

	auto last_rva = options.directory_rva;
	for (const auto& basereloc : relocs)
	{
		last_rva = (std::max)(last_rva, struct_to_file_offset(instance,
			basereloc.get_descriptor(), true, options.write_virtual_part));

		for (const auto& entry : basereloc.get_relocations())
		{
			last_rva = (std::max)(last_rva, struct_to_file_offset(instance,
				entry.get_descriptor(), true, options.write_virtual_part));
			if (entry.get_param().has_value())
			{
				last_rva = (std::max)(last_rva, struct_to_file_offset(instance,
					*entry.get_param(), true));
			}
		}
	}

	update_data_directory(instance, options, last_rva - options.directory_rva);
}

template<typename Directory>
std::uint32_t build_new_impl(buffers::output_buffer_interface& buf, Directory& relocs,
	const builder_options& options)
{
	assert(options.directory_rva);

	auto base_wpos = buf.wpos();
	packed_struct<std::uint16_t> param;
	relocation_entry absolute;
	for (auto& basereloc : relocs)
	{
		auto elem_count = get_element_count(basereloc);
		bool requires_alignment = options.align_base_relocation_structures && (elem_count % 2);

		auto& descriptor = basereloc.get_descriptor();
		utilities::safe_uint<std::uint32_t> size;
		size += descriptor.packed_size;
		size += elem_count * Directory::value_type::entry_list_type
			::value_type::packed_descriptor_type::packed_size;
		size += requires_alignment * sizeof(detail::relocations::type_or_offset_entry);

		descriptor->size_of_block = size.value();
		descriptor.serialize(buf, true);

		for (auto& entry : basereloc.get_relocations())
		{
			entry.get_descriptor().serialize(buf, true);
			if (entry.get_param().has_value())
			{
				param = *entry.get_param();
				param.serialize(buf, true);
			}
		}

		if (requires_alignment)
			absolute.get_descriptor().serialize(buf, true);
	}
	
	return static_cast<std::uint32_t>(buf.wpos() - base_wpos);
}

template<typename Directory>
std::uint32_t build_new_impl(image::image& instance, Directory& directory,
	const builder_options& options)
{
	assert(options.directory_rva);
	auto buf = buffers::output_memory_ref_buffer(section_data_from_rva(instance, options.directory_rva, true));
	auto result = build_new_impl(buf, directory, options);
	update_data_directory(instance, options, result);
	return result;
}

} //namespace

namespace pe_bliss::relocations
{

void build_in_place(image::image& instance, const base_relocation_details_list& directory,
	const builder_options& options)
{
	build_in_place_impl(instance, directory, options);
}

void build_in_place(image::image& instance, const base_relocation_list& directory,
	const builder_options& options)
{
	build_in_place_impl(instance, directory, options);
}

std::uint32_t build_new(image::image& instance, base_relocation_details_list& directory,
	const builder_options& options)
{
	return build_new_impl(instance, directory, options);
}

std::uint32_t build_new(image::image& instance, base_relocation_list& directory,
	const builder_options& options)
{
	return build_new_impl(instance, directory, options);
}

std::uint32_t build_new(buffers::output_buffer_interface& buf, base_relocation_details_list& directory,
	const builder_options& options)
{
	return build_new_impl(buf, directory, options);
}

std::uint32_t build_new(buffers::output_buffer_interface& buf, base_relocation_list& directory,
	const builder_options& options)
{
	return build_new_impl(buf, directory, options);
}

std::uint32_t get_built_size(const base_relocation_details_list& directory,
	const builder_options& options) noexcept
{
	return get_built_size_impl(directory, options);
}

std::uint32_t get_built_size(const base_relocation_list& directory,
	const builder_options& options) noexcept
{
	return get_built_size_impl(directory, options);
}

} //namespace pe_bliss::relocations
