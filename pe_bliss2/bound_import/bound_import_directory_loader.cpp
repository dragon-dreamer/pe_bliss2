#include "pe_bliss2/bound_import/bound_import_directory_loader.h"

#include <system_error>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/string_from_va.h"
#include "pe_bliss2/image/struct_from_va.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/math.h"

namespace
{

struct bound_import_directory_loader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "bound_import_directory_loader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::bound_import::bound_import_directory_loader_errc;
		switch (static_cast<pe_bliss::bound_import::bound_import_directory_loader_errc>(ev))
		{
		case invalid_library_name:
			return "Invalid bound import library name";
		case name_offset_overlaps_descriptors:
			return "Library name offset overlaps bound import descriptors";
		case invalid_bound_import_entry:
			return "Invalid bound import entry";
		default:
			return {};
		}
	}
};

const bound_import_directory_loader_error_category bound_import_directory_loader_error_category_instance;

using namespace pe_bliss;
using namespace pe_bliss::bound_import;

template<typename ListElem>
rva_type read_bound_import_entry(rva_type current_rva, rva_type start_rva,
	const image::image& instance, const loader_options& options, ListElem& elem)
{
	auto& descriptor = elem.get_descriptor();
	struct_from_rva(instance, current_rva, descriptor,
		options.include_headers, options.allow_virtual_data);
	current_rva += descriptor.packed_size;
	if (!descriptor->offset_module_name)
		return current_rva;

	auto name_rva = start_rva;
	if (!utilities::math::add_if_safe<rva_type>(name_rva, descriptor->offset_module_name))
	{
		elem.add_error(bound_import_directory_loader_errc::invalid_library_name);
		return current_rva;
	}

	try
	{
		string_from_rva(instance, name_rva, elem.get_library_name(),
			options.include_headers, options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		elem.add_error(bound_import_directory_loader_errc::invalid_library_name);
	}
	return current_rva;
}

template<typename Entry>
void check_name_offset(Entry& entry, std::uint32_t descriptors_end_offset)
{
	if (entry.has_errors())
		return;

	auto offset = entry.get_descriptor()->offset_module_name;
	if (offset < descriptors_end_offset)
		entry.add_error(bound_import_directory_loader_errc::name_offset_overlaps_descriptors);
}

bool load_next_entry(rva_type start_rva, const image::image& instance,
	const loader_options& options, bound_library_details_list& list,
	rva_type& current_rva, std::uint32_t& descriptor_count)
{
	auto& elem = list.emplace_back();
	const auto& descriptor = elem.get_descriptor();
	try
	{
		current_rva = read_bound_import_entry(
			current_rva, start_rva, instance, options, elem);
		++descriptor_count;
	}
	catch (const std::system_error&)
	{
		elem.add_error(bound_import_directory_loader_errc::invalid_bound_import_entry);
		return false;
	}

	if (!descriptor->offset_module_name)
	{
		list.pop_back();
		--descriptor_count;
		return false;
	}

	auto& references = elem.get_references();
	for (std::uint32_t i = 0; i != descriptor->number_of_module_forwarder_refs; ++i)
	{
		auto& ref = references.emplace_back();
		try
		{
			current_rva = read_bound_import_entry(
				current_rva, start_rva, instance, options, ref);
		}
		catch (const std::system_error&)
		{
			ref.add_error(bound_import_directory_loader_errc::invalid_bound_import_entry);
			return false;
		}

		if (!ref.get_descriptor()->offset_module_name)
			ref.add_error(bound_import_directory_loader_errc::invalid_library_name);
	}

	descriptor_count += descriptor->number_of_module_forwarder_refs;
	return true;
}

} //namespace

namespace pe_bliss::bound_import
{

std::error_code make_error_code(bound_import_directory_loader_errc e) noexcept
{
	return { static_cast<int>(e), bound_import_directory_loader_error_category_instance };
}

std::optional<bound_library_details_list> load(const image::image& instance,
	const loader_options& options)
{
	std::optional<bound_library_details_list> result;
	if (!instance.get_data_directories().has_bound_import())
		return result;

	auto& list = result.emplace();

	auto start_rva = instance.get_data_directories().get_directory(
		core::data_directories::directory_type::bound_import)->virtual_address;
	auto current_rva = start_rva;
	std::uint32_t descriptor_count = 0;
	while (load_next_entry(start_rva, instance, options,
		list, current_rva, descriptor_count))
	{
	}

	auto descriptors_end_offset = (descriptor_count + 1u)
		* static_cast<std::uint32_t>(
			bound_library_details_list::value_type::packed_descriptor_type::packed_size);
	for (auto& entry : list)
	{
		check_name_offset(entry, descriptors_end_offset);
		for (auto& ref : entry.get_references())
			check_name_offset(ref, descriptors_end_offset);
	}

	return result;
}

} //namespace pe_bliss::bound_import
