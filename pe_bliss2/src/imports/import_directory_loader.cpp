#include "pe_bliss2/imports/import_directory_loader.h"

#include <limits>
#include <system_error>
#include <type_traits>
#include <variant>
#include <vector>

#include "pe_bliss2/delay_import/delay_import_directory_loader.h"
#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/string_from_va.h"
#include "pe_bliss2/image/struct_from_va.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/pe_types.h"
#include "utilities/safe_uint.h"

namespace
{

struct import_directory_loader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "import_directory_loader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::imports::import_directory_loader_errc;
		switch (static_cast<pe_bliss::imports::import_directory_loader_errc>(ev))
		{
		case invalid_library_name:
			return "Invalid imported library name";
		case invalid_import_hint:
			return "Invalid import hint";
		case invalid_import_name:
			return "Invalid import name";
		case invalid_hint_name_rva:
			return "Invalid import hint and name RVA";
		case lookup_and_address_table_thunks_differ:
			return "Import lookup and address table thunks differ";
		case invalid_import_directory:
			return "Invalid import directory";
		case zero_iat_and_ilt:
			return "Both import address table and import lookup table pointers are zero";
		case invalid_import_ordinal:
			return "Invalid import ordinal";
		case zero_iat:
			return "Import address table pointer is zero";
		case invalid_imported_library_iat_ilt:
			return "Invalid imported library import address table or import lookup table";
		case empty_library_name:
			return "Empty imported library name";
		case empty_import_name:
			return "Empty import name";
		case address_and_unload_table_thunks_differ:
			return "Delayload import address and unload table thunks differ";
		default:
			return {};
		}
	}
};

const import_directory_loader_error_category import_directory_loader_error_category_instance;

using namespace pe_bliss;
using namespace pe_bliss::imports;

template<typename ImportList, typename Directory>
rva_type load_library(const image::image& instance, const loader_options& options,
	rva_type current_descriptor_rva, ImportList& import_list, Directory& directory)
{
	auto& library = import_list.emplace_back();
	auto& descriptor = library.get_descriptor();

	try
	{
		struct_from_rva(instance, current_descriptor_rva, descriptor,
			options.include_headers, options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		import_list.pop_back();
		directory.add_error(import_directory_loader_errc::invalid_import_directory);
		return 0u;
	}

	if (!descriptor->name)
	{
		import_list.pop_back();
		return 0u;
	}

	try
	{
		string_from_rva(instance, descriptor->name, library.get_library_name(),
			options.include_headers, options.allow_virtual_data);

		if (library.get_library_name().value().empty())
			library.add_error(import_directory_loader_errc::empty_library_name);
	}
	catch (const std::system_error&)
	{
		library.add_error(import_directory_loader_errc::invalid_library_name);
	}

	if (!utilities::math::add_if_safe(current_descriptor_rva,
		static_cast<rva_type>(descriptor.packed_size)))
	{
		directory.add_error(import_directory_loader_errc::invalid_import_directory);
		return 0u;
	}

	return current_descriptor_rva;
}

bool is_ordinal(std::uint64_t thunk) noexcept
{
	return static_cast<bool>(thunk & detail::imports::image_ordinal_flag64);
}

bool is_ordinal(std::uint32_t thunk) noexcept
{
	return static_cast<bool>(thunk & detail::imports::image_ordinal_flag32);
}

std::uint64_t to_ordinal(std::uint64_t thunk) noexcept
{
	return thunk & ~detail::imports::image_ordinal_flag64;
}

std::uint32_t to_ordinal(std::uint32_t thunk) noexcept
{
	return thunk & ~detail::imports::image_ordinal_flag32;
}

template<typename Import, typename Va, typename Descriptor>
void add_imported_va(const image::image& instance,
	Import& new_import, imported_library_details<Va, Descriptor>& library)
{
	if ((library.has_lookup_table() && library.is_bound()) || instance.is_loaded_to_memory())
	{
		std::visit([&new_import] (auto& info) {
			info.get_imported_va() = new_import.get_address(); },
			new_import.get_import_info());
	}
}

template<typename Va, typename Descriptor>
bool load_import(const image::image& instance, const loader_options& options,
	utilities::safe_uint<rva_type>& lookup_rva,
	utilities::safe_uint<rva_type>& address_rva,
	[[maybe_unused]] utilities::safe_uint<rva_type>& unload_rva,
	imported_library_details<Va, Descriptor>& library)
{
	auto& imports = library.get_imports();
	auto& new_import = imports.emplace_back();

	try
	{
		if (lookup_rva)
		{
			struct_from_rva(instance, lookup_rva.value(), new_import.get_lookup().emplace(),
				options.include_headers, options.allow_virtual_data);
		}

		if constexpr (imported_library_details<Va, Descriptor>::is_delayload)
		{
			if (unload_rva)
			{
				struct_from_rva(instance, unload_rva.value(), new_import.get_unload_entry().emplace(),
					options.include_headers, options.allow_virtual_data);
			}
		}

		struct_from_rva(instance, address_rva.value(), new_import.get_address(),
			options.include_headers, options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		imports.pop_back();
		library.add_error(import_directory_loader_errc::invalid_imported_library_iat_ilt);
		return false;
	}

	using packed_va_type = typename imported_address<Va,
		imported_library_details<Va, Descriptor>::is_delayload>::packed_va_type;
	if (!new_import.get_lookup().value_or(packed_va_type{}).get()
		&& !new_import.get_address().get())
	{
		imports.pop_back();
		return false;
	}

	if constexpr (!imported_library_details<Va, Descriptor>::is_delayload)
	{
		if (lookup_rva && !instance.is_loaded_to_memory() && !library.is_bound())
		{
			if (new_import.get_lookup()->get() != new_import.get_address().get())
				new_import.add_error(import_directory_loader_errc::lookup_and_address_table_thunks_differ);
		}

		if (!lookup_rva && instance.is_loaded_to_memory())
		{
			new_import.get_import_info().template emplace<imported_function_address<Va>>()
				.get_imported_va() = new_import.get_address();
			return true;
		}
	}
	else
	{
		if (unload_rva)
		{
			if (new_import.get_address().get() != new_import.get_unload_entry()->get())
				new_import.add_error(import_directory_loader_errc::address_and_unload_table_thunks_differ);
		}
	}

	auto thunk = new_import.get_lookup()
		? new_import.get_lookup()->get() : new_import.get_address().get();

	if (is_ordinal(thunk))
	{
		auto& info = new_import.get_import_info()
			.template emplace<imported_function_ordinal<Va>>();
		add_imported_va(instance, new_import, library);

		auto ordinal = to_ordinal(thunk);
		if (ordinal > (std::numeric_limits<ordinal_type>::max)())
			new_import.add_error(import_directory_loader_errc::invalid_import_ordinal);
		info.set_ordinal(static_cast<ordinal_type>(ordinal));
		return true;
	}

	auto& info = new_import.get_import_info()
		.template emplace<imported_function_hint_and_name<Va>>();
	add_imported_va(instance, new_import, library);
	utilities::safe_uint<rva_type> hint_name_rva;
	try
	{
		hint_name_rva += thunk;
	}
	catch (const std::system_error&)
	{
		new_import.add_error(import_directory_loader_errc::invalid_hint_name_rva);
		return true;
	}

	try
	{
		struct_from_rva(instance, hint_name_rva.value(),
			info.get_hint(), options.include_headers, options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		new_import.add_error(import_directory_loader_errc::invalid_import_hint);
		return true;
	}

	try
	{
		hint_name_rva += imported_function_hint_and_name<Va>::hint_type::packed_size;
		string_from_rva(instance, hint_name_rva.value(),
			info.get_name(), options.include_headers, options.allow_virtual_data);
		if (info.get_name().value().empty())
			new_import.add_error(import_directory_loader_errc::empty_import_name);
	}
	catch (const std::system_error&)
	{
		new_import.add_error(import_directory_loader_errc::invalid_import_name);
	}

	return true;
}

template<typename Va, typename Directory, typename Descriptor>
void load_impl(const image::image& instance, const loader_options& options,
	rva_type current_descriptor_rva,
	std::vector<imported_library_details<Va, Descriptor>>& import_list,
	Directory& directory)
{
	while ((current_descriptor_rva = load_library(
		instance, options, current_descriptor_rva, import_list, directory)) != 0u)
	{
		auto& library = import_list.back();
		auto& descriptor = library.get_descriptor();
		utilities::safe_uint current_lookup_rva = descriptor->lookup_table;
		utilities::safe_uint current_address_rva = descriptor->address_table;
		utilities::safe_uint<rva_type> current_unload_rva;
		if constexpr (imported_library_details<Va, Descriptor>::is_delayload)
			current_unload_rva = descriptor->unload_information_table_rva;

		if (!current_lookup_rva && !current_address_rva)
		{
			library.add_error(import_directory_loader_errc::zero_iat_and_ilt);
			continue;
		}

		if (!current_address_rva)
		{
			library.add_error(import_directory_loader_errc::zero_iat);
			continue;
		}

		while (load_import(instance, options,
			current_lookup_rva, current_address_rva, current_unload_rva, library))
		{
			try
			{
				if (current_lookup_rva)
					current_lookup_rva += sizeof(Va);
				if (current_unload_rva)
					current_unload_rva += sizeof(Va);

				current_address_rva += sizeof(Va);
			}
			catch (const std::system_error&)
			{
				library.add_error(import_directory_loader_errc::invalid_imported_library_iat_ilt);
				return;
			}
		}
	}
}

template<typename Directory>
std::optional<Directory> load_generic(const image::image& instance,
	const loader_options& options)
{
	std::optional<Directory> result;
	if (!instance.get_data_directories().has_directory(options.target_directory))
		return result;

	auto& dir = instance.get_data_directories().get_directory(
		options.target_directory);
	auto imports_rva = dir->virtual_address;
	if (!imports_rva || !dir->size)
		return result;

	auto& directory = result.emplace();
	if (instance.is_64bit())
	{
		load_impl(instance, options, imports_rva,
			directory.get_list().template emplace<
				std::vector<typename Directory::imported_library64_type>>(),
			directory);
	}
	else
	{
		load_impl(instance, options, imports_rva,
			directory.get_list().template emplace<
				std::vector<typename Directory::imported_library32_type>>(),
			directory);
	}

	return result;
}

} //namespace

namespace pe_bliss::imports
{

std::error_code make_error_code(import_directory_loader_errc e) noexcept
{
	return { static_cast<int>(e), import_directory_loader_error_category_instance };
}

std::optional<import_directory_details> load(const image::image& instance,
	const loader_options& options)
{
	return load_generic<import_directory_details>(instance, options);
}

} //namespace pe_bliss::imports

namespace pe_bliss::delay_import
{

std::optional<delay_import_directory_details> load(const image::image& instance,
	const loader_options& options)
{
	return load_generic<delay_import_directory_details>(instance, options);
}

} //namespace pe_bliss::delay_import
