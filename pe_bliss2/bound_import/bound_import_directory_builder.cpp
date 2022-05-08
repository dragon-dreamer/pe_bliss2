#include "pe_bliss2/bound_import/bound_import_directory_builder.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "buffers/output_buffer_interface.h"
#include "buffers/output_memory_ref_buffer.h"
#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/image.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/pe_types.h"
#include "utilities/generic_error.h"
#include "utilities/safe_uint.h"

namespace
{

using namespace pe_bliss;
using namespace pe_bliss::bound_import;

template<typename Directory>
std::uint32_t get_built_size_impl(const Directory& directory) noexcept
{
	utilities::safe_uint<std::uint32_t> result = 0;
	for (const auto& entry : directory)
	{
		result += std::remove_cvref_t<decltype(entry)>::packed_descriptor_type::packed_size;
		result += entry.get_library_name().value().size() + 1u; /* nullbyte */
		for (const auto& ref : entry.get_references())
		{
			result += std::remove_cvref_t<decltype(ref)>::packed_descriptor_type::packed_size;
			result += ref.get_library_name().value().size() + 1u; /* nullbyte */
		}
	}

	//Terminator
	result += Directory::value_type::packed_descriptor_type::packed_size;
	return result.value();
}

template<typename Directory>
std::size_t get_descriptor_count(const Directory& directory) noexcept
{
	std::size_t count = 0;
	for (const auto& entry : directory)
		count += 1 + entry.get_references().size();

	return count;
}

void update_data_directory(image& instance, const builder_options& options, std::uint32_t size)
{
	if (options.update_data_directory)
	{
		auto& dir = instance.get_data_directories().get_directory(
			core::data_directories::directory_type::bound_import);
		dir->virtual_address = options.directory_rva;
		dir->size = size;
	}
}

template<typename Entry>
void build_descriptor_in_place(image& instance,
	const Entry& entry, rva_type& last_descriptor_rva,
	rva_type& last_rva, const builder_options& options)
{
	last_descriptor_rva = (std::max)(last_descriptor_rva,
		instance.struct_to_file_offset(entry.get_descriptor(),
		true, options.write_virtual_part));
	last_rva = (std::max)(last_rva,
		instance.string_to_file_offset(entry.get_library_name(), true, options.write_virtual_part));
	last_rva = (std::max)(last_rva, last_descriptor_rva);
}

template<typename Directory>
void build_in_place_impl(image& instance, const Directory& directory,
	const builder_options& options)
{
	assert(options.directory_rva);

	auto last_descriptor_rva = options.directory_rva;
	auto last_rva = last_descriptor_rva;
	for (const auto& entry : directory)
	{
		build_descriptor_in_place(instance, entry, last_descriptor_rva, last_rva, options);
		for (const auto& ref : entry.get_references())
			build_descriptor_in_place(instance, ref, last_descriptor_rva, last_rva, options);
	}

	last_rva = (std::max)(last_rva, instance.struct_to_rva(last_descriptor_rva,
		detail::bound_import::image_bound_import_descriptor{}, true, true));
	update_data_directory(instance, options, last_rva - options.directory_rva);
}

template<typename Entry>
void write_descriptor(Entry& entry, std::size_t& strings_offset,
	buffers::output_buffer_interface& buf)
{
	if (strings_offset > (std::numeric_limits<std::uint16_t>::max)())
		throw pe_error(utilities::generic_errc::integer_overflow);

	entry.get_descriptor()->offset_module_name = static_cast<std::uint16_t>(strings_offset);
	entry.get_descriptor().serialize(buf, true);
	strings_offset += entry.get_library_name().value().size() + 1;
}

template<typename Directory>
std::uint32_t build_new_impl(buffers::output_buffer_interface& buf, Directory& directory)
{
	auto descriptor_count = get_descriptor_count(directory) + 1;
	auto start_pos = buf.wpos();
	auto strings_offset = descriptor_count
		* Directory::value_type::packed_descriptor_type::packed_size;

	for (auto& entry : directory)
	{
		write_descriptor(entry, strings_offset, buf);

		for (auto& ref : entry.get_references())
			write_descriptor(ref, strings_offset, buf);
	}

	typename Directory::value_type::packed_descriptor_type{}.serialize(buf, true);

	for (auto& entry : directory)
	{
		entry.get_library_name().serialize(buf, true);
		for (auto& ref : entry.get_references())
			ref.get_library_name().serialize(buf, true);
	}

	return static_cast<std::uint32_t>(buf.wpos() - start_pos);
}

template<typename Directory>
std::uint32_t build_new_impl(image& instance, Directory& directory,
	const builder_options& options)
{
	assert(options.directory_rva);
	auto buf = buffers::output_memory_ref_buffer(instance.section_data_from_rva(options.directory_rva, true));
	auto size = static_cast<std::uint32_t>(build_new_impl(buf, directory));
	update_data_directory(instance, options, size);
	return size;
}

} //namespace

namespace pe_bliss::bound_import
{

void build_in_place(image& instance, const bound_library_list& directory,
	const builder_options& options)
{
	build_in_place_impl(instance, directory, options);
}

void build_in_place(image& instance, const bound_library_details_list& directory,
	const builder_options& options)
{
	build_in_place_impl(instance, directory, options);
}

std::uint32_t build_new(image& instance, bound_library_list& directory,
	const builder_options& options)
{
	return build_new_impl(instance, directory, options);
}

std::uint32_t build_new(image& instance, bound_library_details_list& directory,
	const builder_options& options)
{
	return build_new_impl(instance, directory, options);
}

std::uint32_t build_new(buffers::output_buffer_interface& buf, bound_library_list& directory)
{
	return build_new_impl(buf, directory);
}

std::uint32_t build_new(buffers::output_buffer_interface& buf, bound_library_details_list& directory)
{
	return build_new_impl(buf, directory);
}

std::uint32_t get_built_size(const bound_library_list& directory) noexcept
{
	return get_built_size_impl(directory);
}

std::uint32_t get_built_size(const bound_library_details_list& directory) noexcept
{
	return get_built_size_impl(directory);
}

} //namespace pe_bliss::bound_import
