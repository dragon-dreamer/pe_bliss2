#include "pe_bliss2/imports/import_directory_builder.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <list>
#include <type_traits>
#include <variant>

#include "buffers/output_buffer_interface.h"
#include "buffers/output_memory_ref_buffer.h"
#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/image/string_to_va.h"
#include "pe_bliss2/image/struct_to_va.h"
#include "pe_bliss2/packed_struct.h"
#include "utilities/safe_uint.h"

namespace
{

using namespace pe_bliss;
using namespace pe_bliss::imports;

struct thunk_count_names_size
{
	std::uint32_t iat_thunk_count{};
	std::uint32_t ilt_thunk_count{};
	std::uint32_t names_size{};
};

template<template <typename> typename ImportedLibrary, detail::executable_pointer Va>
thunk_count_names_size get_thunk_count_and_names_size(
	const std::vector<ImportedLibrary<Va>>& libraries)
{
	utilities::safe_uint<std::uint32_t> iat_thunk_count;
	utilities::safe_uint<std::uint32_t> ilt_thunk_count;
	utilities::safe_uint<std::uint32_t> names_size;
	for (const auto& library : libraries)
	{
		for (const auto& symbol : library.get_imports())
		{
			const auto& info = symbol.get_import_info();
			using symbol_type = std::remove_cvref_t<decltype(symbol)>;
			if (const auto* ptr = std::get_if<typename symbol_type::hint_name_type>(&info))
			{
				names_size += std::remove_pointer_t<std::remove_cvref_t<decltype(ptr)>>
					::hint_type::packed_size;
				names_size += ptr->get_name().value().size() + 1; //nullbyte
			}
		}

		utilities::safe_uint<std::uint32_t> library_iat_thunk_count;
		utilities::safe_uint<std::uint32_t> library_ilt_thunk_count;
		library_iat_thunk_count += library.get_imports().size();

		if (library.has_lookup_table())
			library_ilt_thunk_count += library_iat_thunk_count + 1u; //Terminator
		else
			library_iat_thunk_count += 1u; //Terminator

		iat_thunk_count += library_iat_thunk_count;
		ilt_thunk_count += library_ilt_thunk_count;
	}

	return {
		.iat_thunk_count = iat_thunk_count.value(),
		.ilt_thunk_count = ilt_thunk_count.value(),
		.names_size = names_size.value()
	};
}

template<typename Va>
std::uint32_t get_aligned_thunk_offset(std::uint32_t base, std::uint32_t offset)
{
	utilities::safe_uint<std::uint32_t> thunk_rva(base);
	thunk_rva += offset;
	auto aligned_thunk_rva = thunk_rva;
	aligned_thunk_rva.align_up(sizeof(Va));
	return aligned_thunk_rva.value() - thunk_rva.value();
}

template<template <detail::executable_pointer> typename ImportedLibrary,
	detail::executable_pointer Va>
built_size get_built_size_impl(const std::vector<ImportedLibrary<Va>>& libraries,
	const builder_options& options)
{
	static constexpr auto descriptor_size = ImportedLibrary<Va>::descriptor_type::packed_size;
	auto descriptors_size = static_cast<std::uint64_t>(descriptor_size)
		* (libraries.size() + 1u); //Terminating descriptor

	utilities::safe_uint<std::uint32_t> directory_size;
	directory_size += descriptors_size;

	auto size_info = get_thunk_count_and_names_size(libraries);
	directory_size += size_info.names_size;
	directory_size += static_cast<std::uint64_t>(size_info.ilt_thunk_count) * sizeof(Va);
	if (!options.iat_rva) //No separate IAT
		directory_size += static_cast<std::uint64_t>(size_info.iat_thunk_count) * sizeof(Va);
	for (const auto& library : libraries)
		directory_size += library.get_library_name().value().size() + 1u; //nullbyte
	
	if (!options.iat_rva || size_info.ilt_thunk_count)
		directory_size += get_aligned_thunk_offset<Va>(options.directory_rva, descriptor_size);

	auto iat_size = static_cast<std::uint32_t>(size_info.iat_thunk_count * sizeof(Va));
	if (options.iat_rva)
		iat_size += get_aligned_thunk_offset<Va>(*options.iat_rva, 0u);

	return built_size
	{
		.directory_size = directory_size.value(),
		.iat_size = iat_size
	};
}

template<template <detail::executable_pointer> typename ImportedLibrary>
built_size get_built_size_impl(const import_directory_base<ImportedLibrary>& directory,
	const builder_options& options)
{
	return std::visit([&options] (const auto& libraries) {
		return get_built_size_impl(libraries, options);
	}, directory.get_list());
}

void update_data_directory(image::image& instance, const builder_options& options,
	const build_result& result)
{
	if (options.update_import_data_directory)
	{
		auto& dir = instance.get_data_directories().get_directory(
			core::data_directories::directory_type::imports);
		dir->virtual_address = options.directory_rva;
		dir->size = result.descriptors_size;
	}
	if (options.update_delayed_import_data_directory)
	{
		auto& dir = instance.get_data_directories().get_directory(
			core::data_directories::directory_type::delay_import);
		dir->virtual_address = options.directory_rva;
		dir->size = result.descriptors_size;
	}
	if (options.update_iat_data_directory)
	{
		auto& dir = instance.get_data_directories().get_directory(
			core::data_directories::directory_type::iat);
		dir->virtual_address = result.iat_rva;
		dir->size = result.iat_size;
	}
}

template<template <detail::executable_pointer> typename ImportedLibrary,
	detail::executable_pointer Va>
void build_in_place_impl(image::image& instance, const std::vector<ImportedLibrary<Va>>& libraries,
	const builder_options& options)
{
	auto last_descriptor_rva = options.directory_rva;
	for (const auto& library : libraries)
	{
		auto rva = struct_to_file_offset(instance, library.get_descriptor(),
			true, options.write_virtual_part);
		last_descriptor_rva = (std::max)(last_descriptor_rva, rva);
	}

	//Terminating descriptor
	last_descriptor_rva = struct_to_rva(instance, last_descriptor_rva,
		typename ImportedLibrary<Va>::descriptor_type{}, true, true);

	rva_type first_iat_rva = (std::numeric_limits<rva_type>::max)(), last_iat_rva{};
	rva_type first_ilt_rva = (std::numeric_limits<rva_type>::max)(), last_ilt_rva{};
	for (const auto& library : libraries)
	{
		string_to_file_offset(instance, library.get_library_name(), true, options.write_virtual_part);

		rva_type first_symbol_iat_rva = (std::numeric_limits<rva_type>::max)(), last_symbol_iat_rva{};
		rva_type first_symbol_ilt_rva = (std::numeric_limits<rva_type>::max)(), last_symbol_ilt_rva{};
		for (const auto& symbol : library.get_imports())
		{
			{
				auto rva = struct_to_file_offset(instance, symbol.get_address(),
					true, options.write_virtual_part);
				first_symbol_iat_rva = (std::min)(first_symbol_iat_rva, rva);
				last_symbol_iat_rva = (std::max)(last_symbol_iat_rva, rva);
			}

			if (symbol.get_lookup())
			{
				auto rva = struct_to_file_offset(instance, *symbol.get_lookup(),
					true, options.write_virtual_part);
				first_symbol_ilt_rva = (std::min)(first_symbol_ilt_rva, rva);
				last_symbol_ilt_rva = (std::max)(last_symbol_ilt_rva, rva);
			}

			const auto& info = symbol.get_import_info();
			using symbol_type = std::remove_cvref_t<decltype(symbol)>;
			if (const auto* ptr = std::get_if<typename symbol_type::hint_name_type>(&info))
			{
				struct_to_file_offset(instance, ptr->get_hint(), true, options.write_virtual_part);
				string_to_file_offset(instance, ptr->get_name(), true, options.write_virtual_part);
			}
		}

		//Terminating thunk
		if (first_symbol_ilt_rva != (std::numeric_limits<rva_type>::max)())
			struct_to_rva(instance, last_symbol_ilt_rva, Va{}, true, true);
		else if (first_symbol_iat_rva != (std::numeric_limits<rva_type>::max)())
			struct_to_rva(instance, last_symbol_iat_rva, Va{}, true, true);

		first_iat_rva = (std::min)(first_iat_rva, first_symbol_iat_rva);
		first_ilt_rva = (std::min)(first_ilt_rva, first_symbol_ilt_rva);
		last_iat_rva = (std::max)(last_iat_rva, static_cast<rva_type>(last_symbol_iat_rva + sizeof(Va)));
		last_ilt_rva = (std::max)(last_ilt_rva, static_cast<rva_type>(last_symbol_ilt_rva + sizeof(Va)));
	}

	if (first_iat_rva == (std::numeric_limits<rva_type>::max)())
		first_iat_rva = last_iat_rva = 0;
	else
		first_iat_rva -= sizeof(Va); //first_iat_rva points to the end of the first IAT VA

	update_data_directory(instance, options, {
		.iat_rva = first_iat_rva,
		.iat_size = last_iat_rva - first_iat_rva,
		//Size of import directory is size of its descriptors (including the terminating one)
		.descriptors_size = last_descriptor_rva - options.directory_rva
	});
}

template<typename Directory>
void build_in_place_impl(image::image& instance, const Directory& directory,
	const builder_options& options)
{
	assert(options.directory_rva);

	std::visit([&instance, &options] (const auto& libraries) {
		build_in_place_impl(instance, libraries, options);
	}, directory.get_list());
}

using safe_rva_type = utilities::safe_uint<rva_type>;

template<template <detail::executable_pointer> typename ImportedLibrary,
	detail::executable_pointer Va>
safe_rva_type build_library_names(buffers::output_buffer_interface& buf,
	std::vector<ImportedLibrary<Va>>& libraries, safe_rva_type strings_rva)
{
	for (auto& library : libraries)
	{
		library.get_descriptor()->name = strings_rva.value();
		strings_rva += library.get_library_name().serialize(buf, true);
	}

	return strings_rva;
}

template<template <detail::executable_pointer> typename ImportedLibrary,
	detail::executable_pointer Va>
safe_rva_type build_address_tables(buffers::output_buffer_interface& buf,
	std::vector<ImportedLibrary<Va>>& libraries,
	safe_rva_type strings_rva, safe_rva_type thunk_rva)
{
	packed_struct<Va> thunk;
	for (auto& library : libraries)
	{
		bool has_lookup = library.has_lookup_table();
		bool is_bound = library.is_bound();
		library.get_descriptor()->address_table = thunk_rva.value();
		if (!has_lookup)
			library.get_descriptor()->lookup_table = 0u;
		for (const auto& symbol : library.get_imports())
		{
			const auto& info = symbol.get_import_info();
			using symbol_type = std::remove_cvref_t<decltype(symbol)>;
			if (const auto* hint_name = std::get_if<typename symbol_type::hint_name_type>(&info); hint_name)
			{
				if (has_lookup && is_bound && hint_name->get_imported_va())
				{
					thunk = hint_name->get_imported_va()->get();
				}
				else
				{
					thunk = strings_rva.value();
					strings_rva += sizeof(ordinal_type)
						+ hint_name->get_name().value().size() + 1u; //nullbyte
				}
			}
			else if (const auto* ordinal = std::get_if<typename symbol_type::ordinal_type>(&info); ordinal)
			{
				if (has_lookup && is_bound && ordinal->get_imported_va())
					thunk = ordinal->get_imported_va()->get();
				else
					thunk = ordinal->to_thunk();
			}
			else if (const auto* imported_function_address = std::get_if<
				typename symbol_type::imported_function_address_type>(&info); imported_function_address)
			{
				if (imported_function_address->get_imported_va())
					thunk = imported_function_address->get_imported_va()->get();
				else
					thunk = 0u;
			}

			thunk_rva += thunk.serialize(buf, true);
		}

		if (!has_lookup)
		{
			thunk = 0u;
			thunk_rva += thunk.serialize(buf, true);
		}
	}

	return thunk_rva;
}

template<template <detail::executable_pointer> typename ImportedLibrary,
	detail::executable_pointer Va>
safe_rva_type build_lookup_tables(buffers::output_buffer_interface& buf,
	std::vector<ImportedLibrary<Va>>& libraries,
	safe_rva_type strings_rva, safe_rva_type thunk_rva)
{
	packed_struct<Va> thunk;
	for (auto& library : libraries)
	{
		if (!library.has_lookup_table())
			continue;
		
		library.get_descriptor()->lookup_table = thunk_rva.value();
		for (const auto& symbol : library.get_imports())
		{
			const auto& info = symbol.get_import_info();
			using symbol_type = std::remove_cvref_t<decltype(symbol)>;
			if (const auto* hint_name = std::get_if<typename symbol_type::hint_name_type>(&info); hint_name)
			{
				thunk = strings_rva.value();
				strings_rva += sizeof(ordinal_type)
					+ hint_name->get_name().value().size() + 1u; //nullbyte
			}
			else if (const auto* ordinal = std::get_if<typename symbol_type::ordinal_type>(&info); ordinal)
			{
				thunk = ordinal->to_thunk();
			}
			else if (const auto* imported_function_address = std::get_if<
				typename symbol_type::imported_function_address_type>(&info); imported_function_address)
			{
				if (imported_function_address->get_imported_va())
					thunk = imported_function_address->get_imported_va()->get();
				else
					thunk = 0u;
			}

			thunk_rva += thunk.serialize(buf, true);
		}

		thunk = 0u;
		thunk_rva += thunk.serialize(buf, true);
	}

	return thunk_rva;
}

template<template <detail::executable_pointer> typename ImportedLibrary,
	detail::executable_pointer Va>
void build_hints_with_names(buffers::output_buffer_interface& buf,
	const std::vector<ImportedLibrary<Va>>& libraries, safe_rva_type strings_rva)
{
	for (auto& library : libraries)
	{
		for (auto& symbol : library.get_imports())
		{
			const auto& info = symbol.get_import_info();
			using symbol_type = std::remove_cvref_t<decltype(symbol)>;
			if (const auto* ptr = std::get_if<typename symbol_type::hint_name_type>(&info))
			{
				strings_rva += ptr->get_hint().serialize(buf, true);
				strings_rva += ptr->get_name().serialize(buf, true);
			}
		}
	}
}

template<template <detail::executable_pointer> typename ImportedLibrary,
	detail::executable_pointer Va>
build_result build_new_impl(buffers::output_buffer_interface& buf,
	buffers::output_buffer_interface* iat_buf,
	std::vector<ImportedLibrary<Va>>& libraries, const builder_options& options)
{
	build_result result{};
	utilities::safe_uint<std::size_t> base_wpos = buf.wpos();
	auto size_info = get_thunk_count_and_names_size(libraries);

	auto descriptor_rva = options.directory_rva;
	safe_rva_type thunk_rva = descriptor_rva;
	thunk_rva += (libraries.size() + 1u)
		* ImportedLibrary<Va>::descriptor_type::packed_size;

	bool has_thunks_together_with_descriptors = size_info.ilt_thunk_count || !options.iat_rva;
	if (has_thunks_together_with_descriptors)
		thunk_rva.align_up(sizeof(Va));

	auto strings_rva = thunk_rva + size_info.ilt_thunk_count * sizeof(Va);
	if (!options.iat_rva)
		strings_rva += size_info.iat_thunk_count * sizeof(Va);

	buf.set_wpos((base_wpos + strings_rva - descriptor_rva).value());
	strings_rva = build_library_names(buf, libraries, strings_rva);

	if (has_thunks_together_with_descriptors)
	{
		buf.set_wpos((base_wpos + thunk_rva - descriptor_rva).value());
		thunk_rva = build_lookup_tables(buf, libraries, strings_rva, thunk_rva);
	}

	if (options.iat_rva)
		thunk_rva = *options.iat_rva;
	auto prev_thunk_rva = thunk_rva.value();
	thunk_rva = build_address_tables(iat_buf ? *iat_buf : buf, libraries, strings_rva, thunk_rva);
	result.iat_rva = thunk_rva.value();
	result.iat_size = thunk_rva.value() - prev_thunk_rva;

	buf.set_wpos((base_wpos + strings_rva - descriptor_rva).value());
	build_hints_with_names(buf, libraries, strings_rva);

	result.full_size = static_cast<std::uint32_t>(buf.wpos() - base_wpos.value());
	buf.set_wpos(base_wpos.value());
	for (auto& library : libraries)
		library.get_descriptor().serialize(buf, true);

	//Terminator
	packed_struct<detail::imports::image_import_descriptor>{}.serialize(buf, true);
	result.descriptors_size = static_cast<std::uint32_t>((libraries.size() + 1u)
		* ImportedLibrary<Va>::descriptor_type::packed_size);

	return result;
}

template<typename Directory>
build_result build_new_impl(buffers::output_buffer_interface& buf,
	buffers::output_buffer_interface* iat_buf, Directory& directory,
	const builder_options& options)
{
	return std::visit([&buf, iat_buf , &options] (auto& libraries) {
		return build_new_impl(buf, iat_buf, libraries, options);
	}, directory.get_list());
}

template<typename Directory>
build_result build_new_impl(image::image& instance, Directory& directory,
	const builder_options& options)
{
	assert(options.directory_rva);
	auto buf = buffers::output_memory_ref_buffer(
		section_data_from_rva(instance, options.directory_rva, true));
	std::optional<buffers::output_memory_ref_buffer> iat_buf;
	if (options.iat_rva)
		iat_buf.emplace(section_data_from_rva(instance , *options.iat_rva, true));

	auto result = build_new_impl(buf, iat_buf ? &*iat_buf : nullptr, directory, options);
	update_data_directory(instance, options, result);
	return result;
}

} //namespace

namespace pe_bliss::imports
{

void build_in_place(image::image& instance, const import_directory_details& directory,
	const builder_options& options)
{
	build_in_place_impl(instance, directory, options);
}

void build_in_place(image::image& instance, const import_directory& directory,
	const builder_options& options)
{
	build_in_place_impl(instance, directory, options);
}

build_result build_new(image::image& instance, import_directory_details& directory,
	const builder_options& options)
{
	return build_new_impl(instance, directory, options);
}

build_result build_new(image::image& instance, import_directory& directory,
	const builder_options& options)
{
	return build_new_impl(instance, directory, options);
}

build_result build_new(buffers::output_buffer_interface& directory_buf,
	buffers::output_buffer_interface* iat_buf, import_directory_details& directory,
	const builder_options& options)
{
	assert(!!iat_buf == !!options.iat_rva);
	return build_new_impl(directory_buf, iat_buf, directory, options);
}

build_result build_new(buffers::output_buffer_interface& directory_buf,
	buffers::output_buffer_interface* iat_buf, import_directory& directory,
	const builder_options& options)
{
	return build_new_impl(directory_buf, iat_buf, directory, options);
}

built_size get_built_size(const import_directory_details& directory, const builder_options& options)
{
	return get_built_size_impl(directory, options);
}

built_size get_built_size(const import_directory& directory, const builder_options& options)
{
	return get_built_size_impl(directory, options);
}

} //namespace pe_bliss::imports
