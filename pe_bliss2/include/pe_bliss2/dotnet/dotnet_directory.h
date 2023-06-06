#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

#include "buffers/input_buffer_interface.h"
#include "buffers/ref_buffer.h"
#include "pe_bliss2/detail/dotnet/image_dotnet_directory.h"
#include "pe_bliss2/detail/packed_struct_base.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/packed_c_string.h"
#include "pe_bliss2/packed_struct.h"

namespace pe_bliss::dotnet
{

enum class dotnet_directory_errc
{
	unable_to_deserialize_header = 1,
	unable_to_deserialize_footer,
	version_length_not_aligned,
	unable_to_deserialize_runtime_version,
	too_long_runtime_version_string,
	unable_to_deserialize_stream_header,
	unable_to_deserialize_stream_data,
	duplicate_stream_names
};

std::error_code make_error_code(dotnet_directory_errc) noexcept;

struct comimage_flags final
{
	comimage_flags() = delete;
	enum value
	{
		ilonly = detail::dotnet::comimage_flags::ilonly,
		x32bitrequired = detail::dotnet::comimage_flags::x32bitrequired,
		il_library = detail::dotnet::comimage_flags::il_library,
		strongnamesigned = detail::dotnet::comimage_flags::strongnamesigned,
		native_entrypoint = detail::dotnet::comimage_flags::native_entrypoint,
		trackdebugdata = detail::dotnet::comimage_flags::trackdebugdata,
		x32bitpreferred = detail::dotnet::comimage_flags::x32bitpreferred
	};
};

template<typename... Bases>
class [[nodiscard]] cor20_header_base
	: public detail::packed_struct_base<detail::dotnet::image_cor20_header>
	, public Bases...
{
public:
	[[nodiscard]]
	comimage_flags::value get_flags() const noexcept
	{
		return static_cast<comimage_flags::value>(descriptor_->flags);
	}

	[[nodiscard]] const buffers::ref_buffer& get_metadata() const& noexcept
	{
		return metadata_;
	}

	[[nodiscard]] buffers::ref_buffer& get_metadata() & noexcept
	{
		return metadata_;
	}

	[[nodiscard]] buffers::ref_buffer get_metadata() && noexcept
	{
		return std::move(metadata_);
	}

	[[nodiscard]] const std::optional<buffers::ref_buffer>& get_resources() const& noexcept
	{
		return resources_;
	}

	[[nodiscard]] std::optional<buffers::ref_buffer>& get_resources() & noexcept
	{
		return resources_;
	}

	[[nodiscard]] std::optional<buffers::ref_buffer> get_resources() && noexcept
	{
		return std::move(resources_);
	}

	[[nodiscard]] const std::optional<buffers::ref_buffer>&
		get_strong_name_signature() const& noexcept
	{
		return strong_name_signature_;
	}

	[[nodiscard]] std::optional<buffers::ref_buffer>&
		get_strong_name_signature() & noexcept
	{
		return strong_name_signature_;
	}

	[[nodiscard]] std::optional<buffers::ref_buffer>
		get_strong_name_signature() && noexcept
	{
		return std::move(strong_name_signature_);
	}

private:
	buffers::ref_buffer metadata_;
	std::optional<buffers::ref_buffer> resources_;
	std::optional<buffers::ref_buffer> strong_name_signature_;
};

using cor20_header = cor20_header_base<>;
using cor20_header_details = cor20_header_base<error_list>;

namespace stream_name
{
constexpr std::string_view utf8_strings("#Strings");
constexpr std::string_view unicode_strings("#US");
constexpr std::string_view blob("#Blob");
constexpr std::string_view guids("#GUID");
constexpr std::string_view metadata_tables("#~");
} //namespace stream_name

template<typename... Bases>
class [[nodiscard]] stream_base
	: public detail::packed_struct_base<detail::dotnet::stream_header>
	, public Bases...
{
public:
	[[nodiscard]] const packed_c_string& get_name() const& noexcept
	{
		return name_;
	}

	[[nodiscard]] packed_c_string& get_name() & noexcept
	{
		return name_;
	}

	[[nodiscard]] packed_c_string get_name() && noexcept
	{
		return std::move(name_);
	}

	[[nodiscard]] const buffers::ref_buffer& get_data() const& noexcept
	{
		return data_;
	}

	[[nodiscard]] buffers::ref_buffer& get_data() & noexcept
	{
		return data_;
	}

	[[nodiscard]] buffers::ref_buffer get_data() && noexcept
	{
		return std::move(data_);
	}

private:
	packed_c_string name_;
	buffers::ref_buffer data_;
};

using stream = stream_base<>;
using stream_details = stream_base<error_list>;

template<typename... Bases>
class [[nodiscard]] metadata_header_base
	: public detail::packed_struct_base<detail::dotnet::metadata_header>
	, public Bases...
{
public:
	using descriptor_footer_type = packed_struct<detail::dotnet::metadata_header_footer>;
	using stream_type = stream_base<Bases...>;
	using stream_list_type = std::vector<stream_type>;

	static constexpr std::uint32_t signature = 0x424a5342u;

public:
	[[nodiscard]]
	descriptor_footer_type& get_descriptor_footer() noexcept
	{
		return footer_;
	}

	[[nodiscard]]
	const descriptor_footer_type& get_descriptor_footer() const noexcept
	{
		return footer_;
	}

	[[nodiscard]] const packed_c_string& get_runtime_version() const& noexcept
	{
		return runtime_version_;
	}

	[[nodiscard]] packed_c_string& get_runtime_version() & noexcept
	{
		return runtime_version_;
	}

	[[nodiscard]] packed_c_string get_runtime_version() && noexcept
	{
		return std::move(runtime_version_);
	}

	[[nodiscard]] const stream_list_type& get_streams() const& noexcept
	{
		return streams_;
	}

	[[nodiscard]] stream_list_type& get_streams() & noexcept
	{
		return streams_;
	}

	[[nodiscard]] stream_list_type get_streams() && noexcept
	{
		return std::move(streams_);
	}

private:
	descriptor_footer_type footer_;
	packed_c_string runtime_version_;
	stream_list_type streams_;
};

using metadata_header = metadata_header_base<>;
using metadata_header_details = metadata_header_base<error_list>;

struct [[nodiscard]] metadata_header_parse_options
{
	std::uint16_t max_runtime_version_length = 10000u;
	bool allow_virtual_data = false;
	bool deserialize_streams = true;
	bool copy_stream_memory = false;
};

[[nodiscard]]
metadata_header_details parse_metadata_header(const buffers::input_buffer_ptr& metadata_buf,
	const metadata_header_parse_options& options = {});

} //namespace pe_bliss::dotnet

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::dotnet::dotnet_directory_errc> : true_type {};
} //namespace std
