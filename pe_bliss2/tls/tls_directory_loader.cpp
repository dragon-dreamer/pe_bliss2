#include "pe_bliss2/tls/tls_directory_loader.h"

#include <algorithm>
#include <limits>
#include <system_error>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/image/section_data_length_from_va.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/pe_types.h"

namespace
{

struct tls_directory_loader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "tls_directory_loader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::tls::tls_directory_loader_errc;
		switch (static_cast<pe_bliss::tls::tls_directory_loader_errc>(ev))
		{
		case invalid_raw_data:
			return "Invalid TLS raw data addresses";
		default:
			return {};
		}
	}
};

const tls_directory_loader_error_category tls_directory_loader_error_category_instance;

using namespace pe_bliss;
using namespace pe_bliss::tls;

template<typename Directory>
Directory load_impl(const image::image& instance, const loader_options& options)
{
	const auto& tls_dir_info = instance.get_data_directories().get_directory(
		core::data_directories::directory_type::tls);

	Directory directory;
	auto& descriptor = directory.get_descriptor();
	instance.struct_from_rva(tls_dir_info->virtual_address, descriptor,
		options.include_headers, options.allow_virtual_data);

	if (descriptor->address_of_callbacks)
	{
		using va_type = typename Directory::va_type;
		packed_struct<va_type> callback_va{};
		while ((callback_va = instance.struct_from_va<va_type>(descriptor->address_of_callbacks,
			options.include_headers, options.allow_virtual_data)).get())
		{
			directory.get_callbacks().emplace_back(callback_va);
		}
	}

	if (descriptor->start_address_of_raw_data)
	{
		if (descriptor->start_address_of_raw_data > descriptor->end_address_of_raw_data
			|| descriptor->end_address_of_raw_data - descriptor->start_address_of_raw_data >
			(std::numeric_limits<std::uint32_t>::max)())
		{
			directory.add_error(tls_directory_loader_errc::invalid_raw_data);
			return directory;
		}

		auto raw_length = section_data_length_from_va(instance,
			descriptor->start_address_of_raw_data, options.include_headers, false);
		raw_length = (std::min)(raw_length,
			static_cast<std::uint32_t>(descriptor->end_address_of_raw_data - descriptor->start_address_of_raw_data));
		if (raw_length)
		{
			auto buf = section_data_from_va(instance, descriptor->start_address_of_raw_data, raw_length,
				options.include_headers, false);
			directory.get_raw_data().deserialize(buf, options.copy_raw_data);
		}
	}

	return directory;
}

} //namespace

namespace pe_bliss::tls
{

std::error_code make_error_code(tls_directory_loader_errc e) noexcept
{
	return { static_cast<int>(e), tls_directory_loader_error_category_instance };
}

std::optional<tls_directory_details> load(const image::image& instance,
	const loader_options& options)
{
	if (!instance.get_data_directories().has_tls())
		return {};

	if (instance.is_64bit())
		return load_impl<tls_directory_details64>(instance, options);
	else
		return load_impl<tls_directory_details32>(instance, options);
}

} //namespace pe_bliss::tls
