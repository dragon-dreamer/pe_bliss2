#include "pe_bliss2/tls/tls_directory_loader.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <system_error>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/image/section_data_length_from_va.h"
#include "pe_bliss2/image/struct_from_va.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/pe_types.h"
#include "utilities/safe_uint.h"

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
		case invalid_directory:
			return "Invalid TLS directory";
		case invalid_callbacks:
			return "Invalid TLS callbacks array";
		case invalid_callback_va:
			return "Invalid TLS callback virtual address";
		default:
			return {};
		}
	}
};

const tls_directory_loader_error_category tls_directory_loader_error_category_instance;

using namespace pe_bliss;
using namespace pe_bliss::tls;

template<typename Directory>
void load_impl(const image::image& instance, const loader_options& options,
	tls_directory_details& dir_holder)
{
	auto& directory = dir_holder.emplace<Directory>();

	auto tls_dir_rva = instance.get_data_directories().get_directory(
		core::data_directories::directory_type::tls)->virtual_address;

	auto& descriptor = directory.get_descriptor();
	try
	{
		struct_from_rva(instance, tls_dir_rva, descriptor,
			options.include_headers, options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		directory.add_error(tls_directory_loader_errc::invalid_directory);
		return;
	}
	
	if (descriptor->address_of_callbacks)
	{
		using va_type = typename Directory::va_type;
		packed_struct<va_type> callback_va{};
		utilities::safe_uint address_of_callback = descriptor->address_of_callbacks;
		try
		{
			while ((callback_va = struct_from_va<va_type>(instance,
				address_of_callback.value(), options.include_headers,
				options.allow_virtual_data)).get())
			{
				auto& callback = directory.get_callbacks().emplace_back(callback_va);

				try
				{
					(void)struct_from_va<std::byte>(instance,
						callback_va.get(), options.include_headers,
						options.allow_virtual_data);
				}
				catch (const std::system_error&)
				{
					callback.add_error(tls_directory_loader_errc::invalid_callback_va);
				}

				address_of_callback += sizeof(va_type);
			}
		}
		catch (const std::system_error&)
		{
			directory.add_error(tls_directory_loader_errc::invalid_callbacks);
		}
	}

	if (descriptor->start_address_of_raw_data)
	{
		if (descriptor->start_address_of_raw_data > descriptor->end_address_of_raw_data
			|| descriptor->end_address_of_raw_data - descriptor->start_address_of_raw_data >
			(std::numeric_limits<std::uint32_t>::max)())
		{
			directory.add_error(tls_directory_loader_errc::invalid_raw_data);
			return;
		}

		try
		{
			auto raw_length = section_data_length_from_va(instance,
				descriptor->start_address_of_raw_data, options.include_headers);
			raw_length = (std::min)(raw_length,
				static_cast<std::uint32_t>(descriptor->end_address_of_raw_data - descriptor->start_address_of_raw_data));
			if (raw_length)
			{
				auto buf = section_data_from_va(instance, descriptor->start_address_of_raw_data, raw_length,
					options.include_headers);
				directory.get_raw_data().deserialize(buf, options.copy_raw_data);
			}
		}
		catch (const std::system_error&)
		{
			directory.add_error(tls_directory_loader_errc::invalid_raw_data);
		}
	}
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
	std::optional<tls_directory_details> result;

	if (!instance.get_data_directories().has_tls())
		return result;

	if (instance.is_64bit())
		load_impl<tls_directory_details64>(instance, options, result.emplace());
	else
		load_impl<tls_directory_details32>(instance, options, result.emplace());

	return result;
}

} //namespace pe_bliss::tls
