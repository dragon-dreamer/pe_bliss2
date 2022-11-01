#include "pe_bliss2/dotnet/dotnet_directory.h"

#include <string>
#include <system_error>
#include <unordered_set>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/input_buffer_section.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/math.h"
#include "utilities/safe_uint.h"

namespace
{
struct dotnet_directory_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "dotnet_directory";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::dotnet::dotnet_directory_errc;
		switch (static_cast<pe_bliss::dotnet::dotnet_directory_errc>(ev))
		{
		case unable_to_deserialize_header:
			return "Unable to deserialize metadata structure header";
		case unable_to_deserialize_footer:
			return "Unable to deserialize metadata structure footer";
		case version_length_not_aligned:
			return "Version length is not aligned to DWORD boundary";
		case unable_to_deserialize_runtime_version:
			return "Unable to deserialize runtime version string";
		case too_long_runtime_version_string:
			return "Too long runtime version string";
		case unable_to_deserialize_stream_header:
			return "Unable to deserialize stream header";
		case unable_to_deserialize_stream_data:
			return "Unable to deserialize stream data";
		case duplicate_stream_names:
			return "Duplicate stream names";
		default:
			return {};
		}
	}
};

const dotnet_directory_error_category dotnet_directory_error_category_instance;
} //namespace

namespace pe_bliss::dotnet
{

std::error_code make_error_code(dotnet_directory_errc e) noexcept
{
	return { static_cast<int>(e), dotnet_directory_error_category_instance };
}

metadata_header_details parse_metadata_header(
	const buffers::input_buffer_ptr& metadata_buf,
	const metadata_header_parse_options& options)
{
	metadata_header_details result;

	buffers::input_buffer_stateful_wrapper ref(metadata_buf);

	try
	{
		result.get_descriptor().deserialize(ref, options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		result.add_error(dotnet_directory_errc::unable_to_deserialize_header);
		return result;
	}

	if (!utilities::math::is_aligned<sizeof(std::uint32_t)>(
		result.get_descriptor()->version_length))
	{
		result.add_error(dotnet_directory_errc::version_length_not_aligned);
	}

	if (result.get_descriptor()->version_length
		> options.max_runtime_version_length)
	{
		result.add_error(dotnet_directory_errc::too_long_runtime_version_string);
		return result;
	}

	try
	{
		result.get_runtime_version().deserialize(ref, options.allow_virtual_data,
			result.get_descriptor()->version_length);
		ref.advance_rpos(static_cast<std::int32_t>(
			result.get_descriptor()->version_length
			- result.get_runtime_version().data_size()));
	}
	catch (const std::system_error&)
	{
		result.add_error(dotnet_directory_errc::unable_to_deserialize_runtime_version);
		return result;
	}

	try
	{
		result.get_descriptor_footer().deserialize(ref, options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		result.add_error(dotnet_directory_errc::unable_to_deserialize_footer);
		return result;
	}

	if (!options.deserialize_streams)
		return result;

	auto stream_count = result.get_descriptor_footer()->stream_count;
	std::unordered_set<std::string_view> stream_names;
	result.get_streams().reserve(stream_count);
	stream_names.reserve(stream_count);
	while (stream_count--)
	{
		auto& stream = result.get_streams().emplace_back();
		try
		{
			stream.get_descriptor().deserialize(ref, options.allow_virtual_data);
			stream.get_name().deserialize(ref, options.allow_virtual_data);
			if (!stream_names.emplace(stream.get_name().value()).second)
				result.add_error(dotnet_directory_errc::duplicate_stream_names);

			utilities::safe_uint rpos = ref.rpos();
			rpos.align_up(sizeof(std::uint32_t));
			ref.set_rpos(rpos.value());
		}
		catch (const std::system_error&)
		{
			stream.add_error(dotnet_directory_errc::unable_to_deserialize_stream_header);
			break;
		}

		try
		{
			stream.get_data().deserialize(buffers::reduce(
				metadata_buf, stream.get_descriptor()->offset, stream.get_descriptor()->size),
				options.copy_stream_memory);
		}
		catch (const std::system_error&)
		{
			stream.add_error(dotnet_directory_errc::unable_to_deserialize_stream_data);
		}
	}

	return result;
}

} //namespace pe_bliss::dotnet
