#include "pe_bliss2/resources/resource_directory_loader.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <system_error>
#include <unordered_set>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/detail/resources/image_resource_directory.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/section_data_length_from_va.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/image/string_from_va.h"
#include "pe_bliss2/image/struct_from_va.h"
#include "pe_bliss2/packed_utf16_string.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/pe_types.h"
#include "utilities/math.h"
#include "utilities/safe_uint.h"
#include "utilities/scoped_guard.h"

namespace
{

struct resource_directory_loader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "resource_directory_loader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::resources::resource_directory_loader_errc;
		switch (static_cast<pe_bliss::resources::resource_directory_loader_errc>(ev))
		{
		case invalid_directory_size:
			return "Invalid resource directory size";
		case invalid_resource_directory:
			return "Invalid resource directory";
		case invalid_resource_directory_number_of_entries:
			return "Invalid resource directory number of entries";
		case invalid_resource_directory_entry:
			return "Invalid resource directory entry";
		case invalid_resource_directory_entry_name:
			return "Invalid resource directory entry name";
		case invalid_number_of_named_and_id_entries:
			return "Invalid number of resource named and ID entries";
		case invalid_resource_data_entry:
			return "Invalid resource data entry";
		case invalid_resource_data_entry_raw_data:
			return "Invalid resource data entry raw data";
		case unsorted_entries:
			return "Unsorted directory entries";
		case duplicate_entries:
			return "Duplicate directory entries";
		default:
			return {};
		}
	}
};

const resource_directory_loader_error_category resource_directory_loader_error_category_instance;

using namespace pe_bliss;
using namespace pe_bliss::resources;

using safe_rva_type = utilities::safe_uint<rva_type>;
using visited_directories_set = std::unordered_set<rva_type>;

bool is_sorted(const std::vector<resource_directory_entry_details>& entries)
{
	return std::is_sorted(entries.cbegin(), entries.cend(),
		[](const auto& l, const auto& r)
	{
		if (l.is_named() == r.is_named())
			return l.get_name_or_id() < r.get_name_or_id();
		else
			return l.is_named();
	});
}

bool has_duplicates(const std::vector<resource_directory_entry_details>& entries)
{
	return std::adjacent_find(entries.cbegin(), entries.cend(),
		[](const auto& l, const auto& r)
	{
		return l.get_name_or_id() == r.get_name_or_id();
	}) != entries.cend();
}

void load_resource_data_entry(const image::image& instance, const loader_options& options,
	safe_rva_type current_rva, std::uint64_t& max_rva, resource_data_entry_details& entry)
{
	auto& entry_descriptor = entry.get_descriptor();

	struct_from_rva(instance,
		current_rva.value(), entry_descriptor, options.include_headers,
		options.allow_virtual_data);

	max_rva = (std::max)(max_rva,
		static_cast<std::uint64_t>(current_rva.value())
		+ entry_descriptor.packed_size);

	try
	{
		auto data_rva = entry_descriptor->offset_to_data;
		auto data_size = entry_descriptor->size;
		max_rva = (std::max)(max_rva,
			static_cast<std::uint64_t>(data_rva) + data_size);
		auto raw_length = section_data_length_from_rva(
			instance, data_rva, options.include_headers, false);
		raw_length = (std::min)(raw_length, data_size);
		if (raw_length)
		{
			auto buf = section_data_from_rva(instance, data_rva, raw_length,
				options.include_headers, options.allow_virtual_data);
			entry.get_raw_data().deserialize(buf, options.copy_raw_data);
		}
	}
	catch (const std::system_error&)
	{
		entry.add_error(
			resource_directory_loader_errc::invalid_resource_data_entry_raw_data);
	}
}

void load_resource_directory(const image::image& instance, const loader_options& options,
	safe_rva_type resource_dir_rva, safe_rva_type current_rva, std::uint64_t& max_rva,
	resource_directory_details& directory, visited_directories_set& visited_directories);

bool load_resource_directory_entry(const image::image& instance, const loader_options& options,
	safe_rva_type resource_dir_rva, safe_rva_type current_rva, std::uint64_t& max_rva,
	resource_directory_entry_details& entry, visited_directories_set& visited_directories)
{
	auto& entry_descriptor = entry.get_descriptor();

	try
	{
		struct_from_rva(instance,
			current_rva.value(), entry_descriptor, options.include_headers,
			options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		entry.add_error(
			resource_directory_loader_errc::invalid_resource_directory_entry);
		return false;
	}

	if (entry_descriptor->name_or_id & detail::resources::name_is_string_flag)
	{
		auto& name = entry.get_name_or_id().emplace<packed_utf16_string>();

		safe_rva_type name_rva;
		try
		{
			name_rva = resource_dir_rva
				+ (entry_descriptor->name_or_id
					& ~detail::resources::name_is_string_flag);
			string_from_rva(instance, name_rva.value(), name,
				options.include_headers, options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			entry.get_name_or_id().emplace<std::monostate>();
			entry.add_error(
				resource_directory_loader_errc::invalid_resource_directory_entry_name);
			return true;
		}
		
		max_rva = (std::max)(max_rva,
			static_cast<std::uint64_t>(name_rva.value()) + name.data_size());
	}
	else
	{
		entry.get_name_or_id().emplace<resource_id_type>(entry_descriptor->name_or_id);
	}

	if (entry_descriptor->offset_to_data_or_directory
		& detail::resources::data_is_directory_flag)
	{
		safe_rva_type dir_rva;

		try
		{
			dir_rva = resource_dir_rva
				+ (entry_descriptor->offset_to_data_or_directory
					& ~detail::resources::data_is_directory_flag);
		}
		catch (const std::system_error&)
		{
			auto& directory = entry.get_data_or_directory().emplace<
				resource_directory_details>();
			directory.add_error(
				resource_directory_loader_errc::invalid_resource_directory);
			return true;
		}

		if (!visited_directories.emplace(dir_rva.value()).second)
		{
			entry.get_data_or_directory().emplace<rva_type>(dir_rva.value());
		}
		else
		{
			load_resource_directory(instance, options,
				resource_dir_rva, dir_rva, max_rva,
				entry.get_data_or_directory().emplace<resource_directory_details>(),
				visited_directories);
		}
	}
	else
	{
		auto& data_entry = entry.get_data_or_directory()
			.emplace<resource_data_entry_details>();
		try
		{
			auto data_entry_rva = resource_dir_rva
				+ entry_descriptor->offset_to_data_or_directory;
			load_resource_data_entry(instance, options,
				data_entry_rva, max_rva, data_entry);
		}
		catch (const std::system_error&)
		{
			data_entry.add_error(
				resource_directory_loader_errc::invalid_resource_data_entry);
		}
	}

	return true;
}

void load_resource_directory(const image::image& instance, const loader_options& options,
	safe_rva_type resource_dir_rva, safe_rva_type current_rva, std::uint64_t& max_rva,
	resource_directory_details& directory, visited_directories_set& visited_directories)
{
	utilities::scoped_guard guard([&visited_directories, current_rva] {
		visited_directories.erase(current_rva.value());
	});

	auto& descriptor = directory.get_descriptor();
	try
	{
		current_rva += static_cast<rva_type>(struct_from_rva(instance,
			current_rva.value(), descriptor, options.include_headers,
			options.allow_virtual_data).packed_size);
	}
	catch (const std::system_error&)
	{
		directory.add_error(
			resource_directory_loader_errc::invalid_resource_directory);
		return;
	}

	max_rva = (std::max<std::uint64_t>)(max_rva, current_rva.value());

	std::uint32_t entry_count = descriptor->number_of_named_entries;
	if (!utilities::math::add_if_safe<std::uint32_t>(entry_count,
		descriptor->number_of_id_entries))
	{
		directory.add_error(
			resource_directory_loader_errc::invalid_resource_directory_number_of_entries);
		return;
	}

	std::uint32_t number_of_named_entries = 0;
	for (std::uint32_t i = 0; i != entry_count; ++i)
	{
		auto& entry = directory.get_entries().emplace_back();
		if (!load_resource_directory_entry(instance, options, resource_dir_rva,
			current_rva, max_rva, entry, visited_directories))
		{
			break;
		}

		number_of_named_entries += entry.is_named();

		try
		{
			current_rva
				+= resources::resource_directory_entry_details::packed_descriptor_type::packed_size;
		}
		catch (const std::system_error&)
		{
			entry.add_error(
				resource_directory_loader_errc::invalid_resource_directory_entry);
			break;
		}
	}

	if (!is_sorted(directory.get_entries()))
	{
		directory.add_error(
			resource_directory_loader_errc::unsorted_entries);
	}
	else if (has_duplicates(directory.get_entries()))
	{
		directory.add_error(
			resource_directory_loader_errc::duplicate_entries);
	}

	if (number_of_named_entries != descriptor->number_of_named_entries)
	{
		directory.add_error(
			resource_directory_loader_errc::invalid_number_of_named_and_id_entries);
	}

	max_rva = (std::max<std::uint64_t>)(max_rva, current_rva.value());
}

} //namespace

namespace pe_bliss::resources
{

std::error_code make_error_code(resource_directory_loader_errc e) noexcept
{
	return { static_cast<int>(e), resource_directory_loader_error_category_instance };
}

std::optional<resource_directory_details> load(const image::image& instance,
	const loader_options& options)
{
	std::optional<resource_directory_details> result;
	if (!instance.get_data_directories().has_resources())
		return result;

	const auto& resource_dir_info = instance.get_data_directories().get_directory(
		core::data_directories::directory_type::resource);

	auto& directory = result.emplace();

	auto last_rva = resource_dir_info->virtual_address;
	if (!utilities::math::add_if_safe(last_rva, resource_dir_info->size))
		directory.add_error(resource_directory_loader_errc::invalid_directory_size);

	visited_directories_set visited_directories{ resource_dir_info->virtual_address };
	std::uint64_t max_rva = resource_dir_info->virtual_address;
	load_resource_directory(instance, options, resource_dir_info->virtual_address,
		resource_dir_info->virtual_address, max_rva, directory, visited_directories);

	if (max_rva > last_rva)
		directory.add_error(resource_directory_loader_errc::invalid_directory_size);

	return result;
}

} //namespace pe_bliss::resources
