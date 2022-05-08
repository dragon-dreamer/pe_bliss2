#include "pe_bliss2/exports/export_directory_builder.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <string_view>
#include <vector>

#include "buffers/output_buffer_interface.h"
#include "buffers/output_memory_ref_buffer.h"
#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/image.h"
#include "pe_bliss2/pe_types.h"
#include "utilities/safe_uint.h"

namespace
{

using namespace pe_bliss;
using namespace pe_bliss::exports;

void update_data_directory(image& instance,
	const builder_options& options, std::uint32_t size)
{
	if (options.update_data_directory)
	{
		auto& dir = instance.get_data_directories().get_directory(
			core::data_directories::directory_type::exports);
		dir->virtual_address = options.directory_rva;
		dir->size = size;
	}
}

template<typename ExportedNamePtr>
struct symbol_ref
{
	ExportedNamePtr name;
	friend bool operator<(const symbol_ref& left, const symbol_ref& right) noexcept
	{
		return left.name->get_name()->value() < right.name->get_name()->value();
	}
};

template<typename ExportList>
auto sort_symbols(ExportList& export_list)
{
	std::vector<symbol_ref<
		std::remove_reference_t<decltype(export_list.front().get_names().front())>*
	>> symbol_names;
	for (auto& symbol : export_list)
	{
		for (auto& name : symbol.get_names())
		{
			if (name.get_name())
				symbol_names.emplace_back(&name);
		}
	}

	std::sort(std::begin(symbol_names), std::end(symbol_names));
	return symbol_names;
}

template<typename Directory>
utilities::safe_uint<rva_type> build_new_functions_impl(buffers::output_buffer_interface& buf,
	Directory& directory, utilities::safe_uint<rva_type> current_rva)
{
	auto& descriptor = directory.get_descriptor();
	descriptor->address_of_functions = current_rva.value();

	auto& export_list = directory.get_export_list();
	if (export_list.empty())
	{
		descriptor->number_of_functions = 0u;
		return current_rva;
	}

	descriptor->number_of_functions = static_cast<std::uint32_t>(export_list.back().get_rva_ordinal() + 1u);
	current_rva += sizeof(rva_type) * descriptor->number_of_functions;
	auto forwarded_name_rva = current_rva;
	std::uint32_t prev_ordinal = 0;
	for (auto& symbol : export_list)
	{
		while (prev_ordinal < symbol.get_rva_ordinal())
		{
			packed_struct<rva_type>{}.serialize(buf, true);
			++prev_ordinal;
		}
		prev_ordinal = symbol.get_rva_ordinal() + 1;

		if (symbol.get_forwarded_name())
		{
			symbol.get_rva() = forwarded_name_rva.value();
			forwarded_name_rva += 
				symbol.get_forwarded_name()->value().size() + 1u; //nullbyte
		}
		symbol.get_rva().serialize(buf, true);
	}

	for (const auto& symbol : export_list)
	{
		if (symbol.get_forwarded_name())
			current_rva += symbol.get_forwarded_name()->serialize(buf, true);
	}

	return current_rva;
}

template<typename Directory>
void build_new_names_impl(buffers::output_buffer_interface& buf,
	Directory& directory, utilities::safe_uint<rva_type> current_rva)
{
	auto& descriptor = directory.get_descriptor();
	descriptor->address_of_name_ordinals = current_rva.value();
	if (directory.get_export_list().empty())
	{
		descriptor->address_of_names = current_rva.value();
		return;
	}

	auto symbol_names = sort_symbols(directory.get_export_list());

	for (const auto& sym : symbol_names)
		current_rva += sym.name->get_name_ordinal().serialize(buf, true);

	descriptor->address_of_names = current_rva.value();

	current_rva += sizeof(rva_type) * symbol_names.size();
	auto name_rva = current_rva;
	for (auto& sym : symbol_names)
	{
		sym.name->get_name_rva() = name_rva.value();
		sym.name->get_name_rva().serialize(buf, true);
		name_rva += sym.name->get_name()->value().size() + 1u; // nullbyte
	}

	for (const auto& sym : symbol_names)
		sym.name->get_name()->serialize(buf, true);
}

template<typename Directory>
std::size_t build_new_impl(buffers::output_buffer_interface& buf, Directory& directory,
	const builder_options& options)
{
	auto& descriptor = directory.get_descriptor();
	utilities::safe_uint current_rva(options.directory_rva);
	current_rva += descriptor.packed_size;
	descriptor->name = current_rva.value();

	auto buf_start_pos = buf.wpos();
	buf.advance_wpos(descriptor.packed_size);
	current_rva += directory.get_library_name().serialize(buf, true);

	directory.get_export_list().sort([] (const auto& l, const auto& r)
		{
			return l.get_rva_ordinal() < r.get_rva_ordinal();
		});

	current_rva = build_new_functions_impl(buf, directory, current_rva);
	build_new_names_impl(buf, directory, current_rva);

	auto size = buf.wpos() - buf_start_pos;
	buf.set_wpos(buf_start_pos);
	descriptor.serialize(buf, true);
	return size;
}

template<typename Directory>
std::uint32_t build_new_impl(image& instance, Directory& directory,
	const builder_options& options)
{
	assert(options.directory_rva);
	auto buf = buffers::output_memory_ref_buffer(instance.section_data_from_rva(options.directory_rva, true));
	auto size = static_cast<std::uint32_t>(build_new_impl(buf, directory, options));
	update_data_directory(instance, options, size);
	return size;
}

template<typename Directory>
std::uint32_t get_built_size_impl(const Directory& directory)
{
	utilities::safe_uint result(static_cast<std::uint32_t>(directory.get_descriptor().packed_size));
	result += directory.get_library_name().value().size() + 1; /* nullbyte */

	//Size of table of functions
	result += directory.get_last_free_ordinal() * sizeof(rva_type);

	for (const auto& symbol : directory.get_export_list())
	{
		if (symbol.get_forwarded_name())
			result += symbol.get_forwarded_name()->value().size() + 1u; /* nullbyte */

		for (const auto& name_info : symbol.get_names())
		{
			if (!name_info.get_name())
				continue;

			//Entry of names and name ordinals tables
			result += sizeof(rva_type) + sizeof(ordinal_type);
			result += name_info.get_name()->value().size() + 1u; /* nullbyte */
		}
	}

	return result.value();
}

template<typename Directory>
rva_type build_functions_in_place(image& instance, const Directory& directory, rva_type last_rva,
	bool write_virtual_part)
{
	const auto& export_list = directory.get_export_list();
	for (const auto& symbol : export_list)
	{
		last_rva = (std::max)(last_rva,
			instance.struct_to_file_offset(symbol.get_rva(), true));
		if (symbol.get_forwarded_name())
		{
			last_rva = (std::max)(last_rva,
				instance.string_to_file_offset(*symbol.get_forwarded_name(), true,
					write_virtual_part));
		}
	}
	return last_rva;
}

template<typename Directory>
rva_type build_names_in_place(image& instance, const Directory& directory, rva_type last_rva,
	bool write_virtual_part)
{
	for (const auto& symbol : directory.get_export_list())
	{
		for (const auto& name : symbol.get_names())
		{
			last_rva = (std::max)(last_rva,
				instance.struct_to_file_offset(name.get_name_rva(), true, write_virtual_part));
			last_rva = (std::max)(last_rva,
				instance.struct_to_file_offset(name.get_name_ordinal(), true, write_virtual_part));
			if (name.get_name())
			{
				last_rva = (std::max)(last_rva,
					instance.string_to_file_offset(*name.get_name(), true, write_virtual_part));
			}
		}
	}

	return last_rva;
}

template<typename Directory>
void build_in_place_impl(image& instance, const Directory& directory,
	const builder_options& options)
{
	const auto& descriptor = directory.get_descriptor();
	auto last_rva = instance.struct_to_file_offset(descriptor, true, options.write_virtual_part);
	last_rva = (std::max)(last_rva,
		instance.string_to_file_offset(directory.get_library_name(), true,
			options.write_virtual_part));

	last_rva = build_functions_in_place(instance, directory, last_rva, options.write_virtual_part);
	last_rva = build_names_in_place(instance, directory, last_rva, options.write_virtual_part);
	std::uint32_t size = last_rva - options.directory_rva;
	update_data_directory(instance, options, size);
}

} //namespace

namespace pe_bliss::exports
{

void build_in_place(image& instance, const export_directory_details& directory,
	const builder_options& options)
{
	build_in_place_impl(instance, directory, options);
}

void build_in_place(image& instance, const export_directory& directory,
	const builder_options& options)
{
	build_in_place_impl(instance, directory, options);
}

std::uint32_t build_new(image& instance, export_directory& directory,
	const builder_options& options)
{
	return build_new_impl(instance, directory, options);
}

std::uint32_t build_new(image& instance, export_directory_details& directory,
	const builder_options& options)
{
	return build_new_impl(instance, directory, options);
}

std::uint32_t build_new(buffers::output_buffer_interface& buf, export_directory& directory,
	const builder_options& options)
{
	return static_cast<std::uint32_t>(build_new_impl(buf, directory, options));
}

std::uint32_t build_new(buffers::output_buffer_interface& buf, export_directory_details& directory,
	const builder_options& options)
{
	return static_cast<std::uint32_t>(build_new_impl(buf, directory, options));
}

std::uint32_t get_built_size(const export_directory& directory)
{
	return get_built_size_impl(directory);
}

std::uint32_t get_built_size(const export_directory_details& directory)
{
	return get_built_size_impl(directory);
}

} //namespace pe_bliss::exports
