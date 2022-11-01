#include "pe_bliss2/exports/export_directory_loader.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/detail/exports/image_export_directory.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/string_from_va.h"
#include "pe_bliss2/image/struct_from_va.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/pe_types.h"
#include "utilities/safe_uint.h"

namespace
{

struct export_directory_loader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "export_directory_loader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::exports::export_directory_loader_errc;
		switch (static_cast<pe_bliss::exports::export_directory_loader_errc>(ev))
		{
		case invalid_library_name:
			return "Invalid exported library name";
		case invalid_forwarded_name:
			return "Invalid exported forwarded name";
		case invalid_name_list:
			return "Invalid exported name list";
		case invalid_address_list:
			return "Invalid exported address list";
		case invalid_name_ordinal:
			return "Invalid exported name ordinal";
		case invalid_name_rva:
			return "Invalid exported name RVA";
		case empty_name:
			return "Empty exported name";
		case unsorted_names:
			return "Unsorted exported names";
		case invalid_rva:
			return "Invalid exported RVA";
		case invalid_directory:
			return "Invalid export directory";
		case invalid_address_list_number_of_functions:
			return "Invalid export address list number of functions";
		case invalid_address_list_number_of_names:
			return "Invalid export address list number of names";
		default:
			return {};
		}
	}
};

const export_directory_loader_error_category export_directory_loader_error_category_instance;

using namespace pe_bliss;
using namespace pe_bliss::exports;

void read_library_name(const image::image& instance,
	const loader_options& options, export_directory_details& directory) try
{
	string_from_rva(instance, directory.get_descriptor()->name,
		directory.get_library_name(),
		options.include_headers, options.allow_virtual_data);
}
catch (const std::system_error&)
{
	directory.add_error(export_directory_loader_errc::invalid_library_name);
}

void read_forwarded_name_or_rva(const image::image& instance,
	const loader_options& options,
	const core::data_directories::packed_struct_type& export_dir_info,
	rva_type exported_addr, exported_address_details& exported_symbol)
{
	if (exported_addr >= export_dir_info->virtual_address
		&& exported_addr + sizeof(rva_type)
		<= export_dir_info->virtual_address + export_dir_info->size)
	{
		try
		{
			string_from_rva(instance, exported_addr,
				exported_symbol.get_forwarded_name().emplace(),
				options.include_headers, options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			exported_symbol.add_error(export_directory_loader_errc::invalid_forwarded_name);
		}
	}
	else
	{
		try
		{
			(void)struct_from_rva<std::byte>(instance, exported_addr,
				options.include_headers, options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			exported_symbol.add_error(export_directory_loader_errc::invalid_rva);
		}
	}
}

using ordinal_to_exported_address_map = std::vector<exported_address_details*>;

ordinal_to_exported_address_map load_addresses(
	const image::image& instance, const loader_options& options,
	const core::data_directories::packed_struct_type& export_dir_info,
	export_directory_details& directory) try
{
	auto& descriptor = directory.get_descriptor();
	auto number_of_functions = (std::min<std::uint32_t>)(descriptor->number_of_functions,
		options.max_number_of_functions);
	if (number_of_functions < descriptor->number_of_functions)
	{
		directory.add_error(
			export_directory_loader_errc::invalid_address_list_number_of_functions);
	}
	utilities::safe_uint address_of_functions = descriptor->address_of_functions;
	auto& export_list = directory.get_export_list();
	export_list.reserve(number_of_functions);
	ordinal_to_exported_address_map ordinal_to_exported_address(number_of_functions);
	for (std::uint32_t i = 0; i != number_of_functions; ++i)
	{
		auto exported_addr = struct_from_rva<rva_type>(instance, address_of_functions.value(),
			options.include_headers, options.allow_virtual_data);
		address_of_functions += sizeof(rva_type);
		if (!exported_addr.get())
			continue;

		auto& exported_symbol = export_list.emplace_back();
		exported_symbol.get_rva() = exported_addr;
		exported_symbol.set_rva_ordinal(static_cast<ordinal_type>(i));
		ordinal_to_exported_address[i] = &exported_symbol;
		read_forwarded_name_or_rva(instance, options, export_dir_info,
			exported_addr.get(), exported_symbol);
	}
	return ordinal_to_exported_address;
}
catch (const std::system_error&)
{
	directory.add_error(export_directory_loader_errc::invalid_address_list);
	return {};
}

auto load_names(const image::image& instance, const loader_options& options,
	export_directory_details& directory,
	const ordinal_to_exported_address_map& ordinal_to_exported_address) try
{
	auto& descriptor = directory.get_descriptor();
	auto number_of_names = (std::min<std::uint32_t>)(descriptor->number_of_names,
		options.max_number_of_names);
	if (number_of_names < descriptor->number_of_names)
	{
		directory.add_error(
			export_directory_loader_errc::invalid_address_list_number_of_names);
	}
	utilities::safe_uint address_of_names = descriptor->address_of_names;
	utilities::safe_uint address_of_name_ordinals = descriptor->address_of_name_ordinals;
	const std::string empty;
	const std::string* prev_name = &empty;
	for (std::uint32_t i = 0; i != number_of_names; ++i)
	{
		auto name_ordinal = struct_from_rva<ordinal_type>(instance,
			address_of_name_ordinals.value(),
			options.include_headers, options.allow_virtual_data);
		auto name_rva = struct_from_rva<rva_type>(instance, address_of_names.value(),
			options.include_headers, options.allow_virtual_data);
		address_of_name_ordinals += sizeof(ordinal_type);
		address_of_names += sizeof(rva_type);

		if (name_ordinal.get() >= ordinal_to_exported_address.size()
			|| !ordinal_to_exported_address[name_ordinal.get()])
		{
			directory.add_error(export_directory_loader_errc::invalid_name_ordinal);
			continue;
		}

		auto addr = ordinal_to_exported_address[name_ordinal.get()];

		optional_c_string name;
		try
		{
			string_from_rva(instance, name_rva.get(), name.emplace(),
				options.include_headers, options.allow_virtual_data);
			if (name.value().value().empty())
				addr->add_error(export_directory_loader_errc::empty_name);
			if (*prev_name > name.value().value())
				directory.add_error(export_directory_loader_errc::unsorted_names);
		}
		catch (const std::system_error&)
		{
			name.reset();
			addr->add_error(export_directory_loader_errc::invalid_name_rva);
		}

		const auto& last_name = addr->get_names().emplace_back(
			std::move(name), name_rva, name_ordinal).get_name();
		if (last_name)
			prev_name = &last_name.value().value();
	}
}
catch (const std::system_error&)
{
	directory.add_error(export_directory_loader_errc::invalid_name_list);
}

} //namespace

namespace pe_bliss::exports
{

std::error_code make_error_code(export_directory_loader_errc e) noexcept
{
	return { static_cast<int>(e), export_directory_loader_error_category_instance };
}

std::optional<export_directory_details> load(const image::image& instance,
	const loader_options& options)
{
	std::optional<export_directory_details> result;
	if (!instance.get_data_directories().has_exports())
		return result;

	const auto& export_dir_info = instance.get_data_directories().get_directory(
		core::data_directories::directory_type::exports);

	auto& directory = result.emplace();

	try
	{
		struct_from_rva(instance, export_dir_info->virtual_address,
			directory.get_descriptor(), options.include_headers, options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		directory.add_error(export_directory_loader_errc::invalid_directory);
		return result;
	}

	read_library_name(instance, options, directory);

	auto ordinal_to_exported_address = load_addresses(
		instance, options, export_dir_info, directory);

	load_names(instance, options, directory, ordinal_to_exported_address);

	return result;
}

} //namespace pe_bliss::exports
