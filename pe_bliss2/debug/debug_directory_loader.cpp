#include "pe_bliss2/debug/debug_directory_loader.h"

#include <string>
#include <system_error>

#include "buffers/input_buffer_section.h"
#include "buffers/ref_buffer.h"
#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/rva_file_offset_converter.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/image/struct_from_va.h"
#include "pe_bliss2/pe_types.h"
#include "utilities/safe_uint.h"

namespace
{
struct debug_directory_loader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "debug_directory_loader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::debug::debug_directory_loader_errc;
		switch (static_cast<pe_bliss::debug::debug_directory_loader_errc>(ev))
		{
		case no_rva_and_file_offset:
			return "Debug data RVA and file offset are both zero";
		case invalid_file_offset:
			return "Invalid debug data file offset";
		case invalid_debug_directory_size:
			return "Invalid debug directory size";
		case excessive_data_in_directory:
			return "Excessive data in debug directory";
		case unable_to_load_entries:
			return "Unable to load debug directory entries";
		case unable_to_load_raw_data:
			return "Unable to load raw debug entry data";
		case rva_and_file_offset_do_not_match:
			return "Debug data RVA and file offset do not match";
		case too_many_debug_directories:
			return "Too many debug directories";
		case too_big_raw_data:
			return "Too big raw debug data";
		default:
			return {};
		}
	}
};

const debug_directory_loader_error_category debug_directory_loader_error_category_instance;

using namespace pe_bliss;
using namespace pe_bliss::debug;

void load_debug_data(const image::image& instance, debug_directory_details& entry,
	const loader_options& options)
{
	const auto& descriptor = entry.get_descriptor().get();
	if (descriptor.size_of_data > options.max_raw_data_size)
	{
		entry.add_error(debug_directory_loader_errc::too_big_raw_data);
		return;
	}

	auto& buf = entry.get_raw_data();
	rva_type rva = descriptor.address_of_raw_data;
	auto file_offset = descriptor.pointer_to_raw_data;
	if (!rva)
	{
		if (!file_offset)
		{
			entry.add_error(debug_directory_loader_errc::no_rva_and_file_offset);
			return;
		}

		try
		{
			rva = file_offset_to_rva(instance, file_offset);
		}
		catch (const std::system_error&)
		{
			//Ignore error, as file_offset could be inside overlay
			//in the end of image
		}

		if (!rva)
		{
			if (!options.include_overlay
				|| !instance.get_overlay().data()->size())
			{
				entry.add_error(debug_directory_loader_errc::invalid_file_offset);
				return;
			}

			auto overlay_offset = instance.get_overlay().data()->absolute_offset();
			if (file_offset < overlay_offset)
			{
				entry.add_error(debug_directory_loader_errc::invalid_file_offset);
				return;
			}

			buf.deserialize(buffers::reduce(instance.get_overlay().data(),
				file_offset - overlay_offset, descriptor.size_of_data),
				options.copy_raw_data);
			return;
		}
	}
	else
	{
		try
		{
			auto converted_rva = file_offset_to_rva(instance, file_offset);
			if (converted_rva != rva)
				entry.add_error(debug_directory_loader_errc::rva_and_file_offset_do_not_match);
		}
		catch (const std::system_error&)
		{
			entry.add_error(debug_directory_loader_errc::rva_and_file_offset_do_not_match);
		}
	}

	auto data = section_data_from_rva(instance, rva,
		descriptor.size_of_data, options.include_headers, options.allow_virtual_data);
	buf.deserialize(data, options.copy_raw_data);
}
} //namespace

namespace pe_bliss::debug
{

std::error_code make_error_code(debug_directory_loader_errc e) noexcept
{
	return { static_cast<int>(e), debug_directory_loader_error_category_instance };
}

std::optional<debug_directory_list_details> load(const image::image& instance,
	const loader_options& options)
{
	std::optional<debug_directory_list_details> result;
	if (!instance.get_data_directories().has_debug())
		return result;

	const auto& dir = instance.get_data_directories().get_directory(
		core::data_directories::directory_type::debug).get();
	auto& list = result.emplace();

	utilities::safe_uint start_rva = dir.virtual_address;
	auto end_rva = start_rva;
	try
	{
		end_rva += dir.size;
		end_rva
			-= debug_directory_list_details::list_type::value_type::descriptor_type::packed_size;
	}
	catch (const std::system_error&)
	{
		list.add_error(debug_directory_loader_errc::invalid_debug_directory_size);
		return result;
	}

	auto max_debug_directories = options.max_debug_directories;
	while (start_rva <= end_rva)
	{
		if (!max_debug_directories--)
		{
			list.add_error(debug_directory_loader_errc::too_many_debug_directories);
			return result;
		}

		auto& entry = list.get_entries().emplace_back();
		try
		{
			struct_from_rva(instance, start_rva.value(), entry.get_descriptor(),
				options.include_headers, options.allow_virtual_data);
			start_rva
				+= debug_directory_list_details::list_type::value_type::descriptor_type::packed_size;
		}
		catch (const std::system_error&)
		{
			list.get_entries().pop_back();
			list.add_error(debug_directory_loader_errc::unable_to_load_entries);
			return result;
		}

		try
		{
			load_debug_data(instance, entry, options);
		}
		catch (const std::system_error&)
		{
			list.add_error(debug_directory_loader_errc::unable_to_load_raw_data);
		}
	}

	if (start_rva != end_rva
		+ debug_directory_list_details::list_type::value_type::descriptor_type::packed_size)
	{
		list.add_error(debug_directory_loader_errc::excessive_data_in_directory);
	}

	return result;
}

} //namespace pe_bliss::debug
