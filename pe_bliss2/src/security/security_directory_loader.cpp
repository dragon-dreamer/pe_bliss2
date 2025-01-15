#include "pe_bliss2/security/security_directory_loader.h"

#include <cstddef>
#include <string>
#include <system_error>

#include "buffers/input_buffer_section.h"
#include "buffers/input_buffer_stateful_wrapper.h"
#include "pe_bliss2/image/image.h"
#include "utilities/math.h"
#include "utilities/safe_uint.h"

namespace
{
struct security_directory_loader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "security_directory_loader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::security_directory_loader_errc;
		switch (static_cast<pe_bliss::security::security_directory_loader_errc>(ev))
		{
		case invalid_directory:
			return "Invalid security directory";
		case invalid_entry:
			return "Invalid security directory entry";
		case invalid_certificate_data:
			return "Invalid security directory certificate data";
		case invalid_entry_size:
			return "Invalid security directory entry size";
		case invalid_directory_size:
			return "Invalid security directory size";
		case unaligned_directory:
			return "Security directory is not aligned";
		case too_many_entries:
			return "Too many securty directory entries";
		default:
			return {};
		}
	}
};

const security_directory_loader_error_category security_directory_loader_error_category_instance;

} //namespace

namespace pe_bliss::security
{

std::error_code make_error_code(security_directory_loader_errc e) noexcept
{
	return { static_cast<int>(e), security_directory_loader_error_category_instance };
}

std::optional<security_directory_details> load(const image::image& instance,
	const loader_options& options)
{
	std::optional<security_directory_details> result;
	if (!instance.get_data_directories().has_security())
		return result;

	const auto& security_dir_info = instance.get_data_directories().get_directory(
		core::data_directories::directory_type::security);

	auto& directory = result.emplace();

	auto overlay_data = instance.get_overlay().data();
	utilities::safe_uint overlay_offset = overlay_data->absolute_offset();
	if (!overlay_data->size() || security_dir_info->virtual_address < overlay_offset)
	{
		directory.add_error(security_directory_loader_errc::invalid_directory);
		return result;
	}

	buffers::input_buffer_stateful_wrapper_ref ref(*overlay_data);
	try
	{
		ref.set_rpos(security_dir_info->virtual_address - overlay_offset.value());
	}
	catch (const std::system_error&)
	{
		directory.add_error(security_directory_loader_errc::invalid_directory);
		return result;
	}

	if (!utilities::math::is_aligned<sizeof(std::uint64_t)>(security_dir_info->virtual_address))
		directory.add_error(security_directory_loader_errc::unaligned_directory);

	std::size_t size = security_dir_info->size;
	static constexpr auto descriptor_size = security_directory_details
		::certificate_entry_list_type::value_type::descriptor_type::packed_size;
	auto max_entries = options.max_entries;
	while (size >= descriptor_size)
	{
		if (!max_entries--)
		{
			directory.add_error(security_directory_loader_errc::too_many_entries);
			return result;
		}

		auto& entry = directory.get_entries().emplace_back();
		try
		{
			entry.get_descriptor().deserialize(ref, false);
			overlay_offset += descriptor_size;
		}
		catch (const std::system_error&)
		{
			entry.add_error(security_directory_loader_errc::invalid_entry);
			return result;
		}

		size -= descriptor_size;
		auto entry_size = entry.get_descriptor()->length;
		if (!entry_size)
		{
			directory.get_entries().pop_back();
			return result;
		}

		if (entry_size < descriptor_size)
		{
			entry.add_error(security_directory_loader_errc::invalid_entry_size);
			continue;
		}

		auto certificate_size = entry_size - descriptor_size;
		if (certificate_size)
		{
			if (certificate_size > size)
			{
				entry.add_error(security_directory_loader_errc::invalid_entry_size);
				return result;
			}

			try
			{
				entry.get_certificate().deserialize(buffers::reduce(overlay_data,
					ref.rpos(), certificate_size), options.copy_raw_data);
				ref.advance_rpos(static_cast<std::int32_t>(certificate_size));
			}
			catch (const std::system_error&)
			{
				entry.add_error(security_directory_loader_errc::invalid_certificate_data);
				return result;
			}
		}

		size -= certificate_size;

		if (size)
		{
			try
			{
				overlay_offset += certificate_size;
				auto old_offset = overlay_offset.value();
				overlay_offset.align_up(sizeof(std::uint64_t));
				std::size_t alignment_size = overlay_offset.value() - old_offset;
				if (size < alignment_size)
					break;
				ref.advance_rpos(static_cast<std::int32_t>(alignment_size));
				size -= alignment_size;
			}
			catch (const std::system_error&)
			{
				entry.add_error(security_directory_loader_errc::invalid_entry);
				return result;
			}
		}
	}

	if (size)
		directory.add_error(security_directory_loader_errc::invalid_directory_size);

	return result;
}

} //namespace pe_bliss::security
