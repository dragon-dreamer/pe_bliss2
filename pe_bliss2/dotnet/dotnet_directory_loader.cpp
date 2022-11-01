#include "pe_bliss2/dotnet/dotnet_directory_loader.h"

#include <string>
#include <system_error>

#include "buffers/ref_buffer.h"
#include "pe_bliss2/detail/image_data_directory.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/image/struct_from_va.h"

namespace
{
struct dotnet_directory_loader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "dotnet_directory_loader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::dotnet::dotnet_directory_loader_errc;
		switch (static_cast<pe_bliss::dotnet::dotnet_directory_loader_errc>(ev))
		{
		case invalid_directory:
			return "Invalid .NET directory";
		case descriptor_and_directory_sizes_do_not_match:
			return ".NET descriptor and directory sizes do not match";
		case unsupported_descriptor_size:
			return "Unsupported .NET descriptor size";
		case empty_metadata:
			return "Empty .NET metadata";
		case unable_to_load_metadata:
			return "Unable to load .NET metadata";
		case unable_to_load_resources:
			return "Unable to load .NET resources";
		case unable_to_load_strong_name_signature:
			return "Unable to load .NET strong name signature";
		case virtual_metadata:
			return ".NET metadata is virtual";
		case virtual_resources:
			return ".NET resources are virtual";
		case virtual_strong_name_signature:
			return ".NET strong name signature is virtual";
		default:
			return {};
		}
	}
};

const dotnet_directory_loader_error_category dotnet_directory_loader_error_category_instance;

void load_buffer(const pe_bliss::image::image& instance,
	pe_bliss::dotnet::cor20_header_details& directory,
	buffers::ref_buffer& buf,
	const pe_bliss::detail::image_data_directory& dir,
	pe_bliss::dotnet::dotnet_directory_loader_errc load_error,
	pe_bliss::dotnet::dotnet_directory_loader_errc virtual_data_error,
	const pe_bliss::dotnet::loader_options& options,
	bool copy_memory)
{
	try
	{
		auto data = pe_bliss::image::section_data_from_rva(instance, dir.virtual_address,
			dir.size, options.include_headers, options.allow_virtual_data);
		buf.deserialize(data, copy_memory);

		if (!options.allow_virtual_data && data->virtual_size())
			directory.add_error(virtual_data_error);
	}
	catch (const std::system_error&)
	{
		directory.add_error(load_error);
		return;
	}
}

void load_buffer(const pe_bliss::image::image& instance,
	pe_bliss::dotnet::cor20_header_details& directory,
	std::optional<buffers::ref_buffer>& buf,
	const pe_bliss::detail::image_data_directory& dir,
	pe_bliss::dotnet::dotnet_directory_loader_errc load_error,
	pe_bliss::dotnet::dotnet_directory_loader_errc virtual_data_error,
	const pe_bliss::dotnet::loader_options& options,
	bool copy_memory)
{
	if (!dir.size || !dir.virtual_address)
		return;

	load_buffer(instance, directory,
		buf.emplace(), dir, load_error, virtual_data_error,
		options, copy_memory);
}

} //namespace

namespace pe_bliss::dotnet
{

std::error_code make_error_code(dotnet_directory_loader_errc e) noexcept
{
	return { static_cast<int>(e), dotnet_directory_loader_error_category_instance };
}

std::optional<cor20_header_details> load(const image::image& instance,
	const loader_options& options)
{
	std::optional<cor20_header_details> result;
	if (!instance.get_data_directories().is_dotnet())
		return result;

	const auto& dotnet_dir_info = instance.get_data_directories().get_directory(
		core::data_directories::directory_type::com_descriptor);

	auto& directory = result.emplace();

	try
	{
		struct_from_rva(instance, dotnet_dir_info->virtual_address,
			directory.get_descriptor(), options.include_headers,
			options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		directory.add_error(dotnet_directory_loader_errc::invalid_directory);
		return result;
	}

	if (dotnet_dir_info->size != directory.get_descriptor()->cb)
		directory.add_error(dotnet_directory_loader_errc::descriptor_and_directory_sizes_do_not_match);

	if (directory.get_descriptor()->cb != cor20_header_details::descriptor_type::packed_size)
		directory.add_error(dotnet_directory_loader_errc::unsupported_descriptor_size);

	const auto& meta_data = directory.get_descriptor()->meta_data;
	if (!meta_data.virtual_address || !meta_data.size)
		directory.add_error(dotnet_directory_loader_errc::empty_metadata);

	load_buffer(instance, directory, directory.get_metadata(),
		meta_data,
		dotnet_directory_loader_errc::unable_to_load_metadata,
		dotnet_directory_loader_errc::virtual_metadata,
		options, options.copy_metadata_memory);
	load_buffer(instance, directory, directory.get_resources(),
		directory.get_descriptor()->resources,
		dotnet_directory_loader_errc::unable_to_load_resources,
		dotnet_directory_loader_errc::virtual_resources,
		options, options.copy_resource_memory);
	load_buffer(instance, directory, directory.get_strong_name_signature(),
		directory.get_descriptor()->strong_name_signature,
		dotnet_directory_loader_errc::unable_to_load_strong_name_signature,
		dotnet_directory_loader_errc::virtual_strong_name_signature,
		options, options.copy_strong_name_signature_memory);

	return result;
}

} //namespace pe_bliss::dotnet
