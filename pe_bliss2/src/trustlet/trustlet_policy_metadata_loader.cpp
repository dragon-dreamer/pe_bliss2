#include "pe_bliss2/trustlet/trustlet_policy_metadata_loader.h"

#include <cstddef>
#include <span>

#include "buffers/input_buffer_interface.h"

#include "pe_bliss2/detail/trustlet/image_policy_metadata.h"
#include "pe_bliss2/exports/export_directory.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_section_search.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/pe_types.h"
#include "pe_bliss2/section/section_header.h"

namespace pe_bliss::trustlet
{

namespace
{
template<typename ExportDirectory>
std::optional<rva_type> get_image_policy_metadata_rva(const ExportDirectory& export_dir)
{
	const auto& export_list = export_dir.get_export_list();
	auto it = export_dir.symbol_by_name(
		detail::trustlet::image_policy_metadata_name_win10_16215);
	if (it == export_list.end())
	{
		it = export_dir.symbol_by_name(
			detail::trustlet::image_policy_metadata_name_win10_16193);
	}

	if (it == export_list.end())
		return {};

	return it->get_rva().get();
}

template<typename Str>
void decode_policy_string(const image::image& instance,
	trustlet_policy_entry_details& entry,
	Str& str)
{
	try
	{
		auto str_data = image::section_data_from_va(instance,
			entry.get_descriptor()->value);
		buffers::input_buffer_stateful_wrapper_ref str_data_wrapper(*str_data);
		str.deserialize(str_data_wrapper, false);
	}
	catch (const std::system_error&)
	{
		entry.add_error(trustlet_policy_errc::invalid_string_address);
	}
}

void decode_policy_value(const image::image& instance,
	trustlet_policy_entry_details& entry)
{
	const auto& entry_descriptor = entry.get_descriptor().get();
	switch (entry.get_type())
	{
	case image_policy_entry_type::boolean:
		entry.get_value().emplace<bool>(
			static_cast<bool>(entry_descriptor.value));
		break;
	case image_policy_entry_type::int8:
		entry.get_value().emplace<std::int8_t>(
			static_cast<std::int8_t>(entry_descriptor.value));
		break;
	case image_policy_entry_type::uint8:
		entry.get_value().emplace<std::uint8_t>(
			static_cast<std::uint8_t>(entry_descriptor.value));
		break;
	case image_policy_entry_type::int16:
		entry.get_value().emplace<std::int16_t>(
			static_cast<std::int16_t>(entry_descriptor.value));
		break;
	case image_policy_entry_type::uint16:
		entry.get_value().emplace<std::uint16_t>(
			static_cast<std::uint16_t>(entry_descriptor.value));
		break;
	case image_policy_entry_type::int32:
		entry.get_value().emplace<std::int32_t>(
			static_cast<std::int32_t>(entry_descriptor.value));
		break;
	case image_policy_entry_type::uint32:
		entry.get_value().emplace<std::uint32_t>(
			static_cast<std::uint32_t>(entry_descriptor.value));
		break;
	case image_policy_entry_type::int64:
		entry.get_value().emplace<std::int64_t>(
			static_cast<std::int64_t>(entry_descriptor.value));
		break;
	case image_policy_entry_type::uint64:
		entry.get_value().emplace<std::uint64_t>(entry_descriptor.value);
		break;

	case image_policy_entry_type::ansi_string:
		decode_policy_string(instance, entry,
			entry.get_value().emplace<packed_c_string>());
		break;

	case image_policy_entry_type::unicode_string:
		decode_policy_string(instance, entry,
			entry.get_value().emplace<packed_utf16_c_string>());
		break;

	case image_policy_entry_type::none:
	case image_policy_entry_type::overriden:
		break;

	default:
		entry.add_error(trustlet_policy_errc::unsupported_entry_type);
		break;
	}
}
} //namespace

template<typename ExportDirectory>
trustlet_check_result is_trustlet(
	const image::image& instance,
	const ExportDirectory& export_dir)
{
	auto metadata_rva = get_image_policy_metadata_rva(export_dir);
	if (!metadata_rva)
		return trustlet_check_result::absent_metadata;

	auto [header_it, data_it] = image::section_from_rva(instance, *metadata_rva,
		trustlet_policy_metadata::descriptor_type::packed_size);

	if (header_it == instance.get_section_table().get_section_headers().end())
		return trustlet_check_result::metadata_in_wrong_section;

	if (header_it->get_name() != detail::trustlet::image_policy_section_name)
		return trustlet_check_result::metadata_in_wrong_section;

	if (header_it->get_characteristics()
		!= (section::section_header::characteristics::cnt_initialized_data
			| section::section_header::characteristics::mem_read))
	{
		return trustlet_check_result::invalid_metadata_section_attributes;
	}

	return trustlet_check_result::valid_trustlet;
}

template<typename ExportDirectory>
std::optional<trustlet_policy_metadata_details> load_trustlet_policy_metadata(
	const image::image& instance,
	const ExportDirectory& export_dir)
{
	std::optional<trustlet_policy_metadata_details> result;

	auto metadata_rva = get_image_policy_metadata_rva(export_dir);
	if (!metadata_rva)
		return result;

	buffers::input_buffer_ptr data;
	try
	{
		data = image::section_data_from_rva(instance, *metadata_rva);
	}
	catch (const std::exception&)
	{
		return result;
	}

	auto& metadata = result.emplace();
	buffers::input_buffer_stateful_wrapper_ref wrapper(*data);
	try
	{
		metadata.get_descriptor().deserialize(wrapper, false);
	}
	catch (const std::system_error& e)
	{
		metadata.add_error(e.code());
		return result;
	}

	if (metadata.get_descriptor()->version
		!= detail::trustlet::image_policy_metadata_version)
	{
		metadata.add_error(trustlet_policy_errc::unsupported_version);
		return result;
	}

	while (true)
	{
		auto& entry = metadata.get_entries().emplace_back();
		try
		{
			entry.get_descriptor().deserialize(wrapper, false);
		}
		catch (const std::system_error& e)
		{
			entry.add_error(e.code());
			break;
		}

		const auto& entry_descriptor = entry.get_descriptor().get();
		if (entry_descriptor.policy_id == 0 && entry_descriptor.type == 0
			&& entry_descriptor.value == 0)
		{
			metadata.get_entries().pop_back();
			break;
		}

		decode_policy_value(instance, entry);
	}

	return result;
}

template std::optional<trustlet_policy_metadata_details>
load_trustlet_policy_metadata<exports::export_directory>(
	const image::image& instance,
	const exports::export_directory& export_dir);
template std::optional<trustlet_policy_metadata_details>
load_trustlet_policy_metadata<exports::export_directory_details>(
	const image::image& instance,
	const exports::export_directory_details& export_dir);

template trustlet_check_result is_trustlet<exports::export_directory>(
	const image::image& instance,
	const exports::export_directory& export_dir);
template trustlet_check_result is_trustlet<exports::export_directory_details>(
	const image::image& instance,
	const exports::export_directory_details& export_dir);

} //namespace pe_bliss::trustlet
