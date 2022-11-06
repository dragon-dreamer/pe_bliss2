#pragma once

#include <utility>
#include <vector>

#include "buffers/ref_buffer.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/detail/resources/icon_cursor.h"

namespace pe_bliss::resources
{

template<typename Header, typename ResourceGroupHeader, std::uint16_t Type>
class [[nodiscard]] icon_cursor_group
{
public:
	static constexpr std::uint16_t type = Type;
	using header_type = packed_struct<Header>;
	using resource_group_header_type = packed_struct<ResourceGroupHeader>;
	using resource_group_header_list_type
		= std::vector<resource_group_header_type>;
	using data_list_type = std::vector<buffers::ref_buffer>;

public:
	[[nodiscard]]
	header_type& get_header() noexcept
	{
		return header_;
	}

	[[nodiscard]]
	const header_type& get_header() const noexcept
	{
		return header_;
	}

	[[nodiscard]]
	resource_group_header_list_type& get_resource_group_headers() & noexcept
	{
		return resource_group_headers_;
	}

	[[nodiscard]]
	const resource_group_header_list_type& get_resource_group_headers() const& noexcept
	{
		return resource_group_headers_;
	}

	[[nodiscard]]
	resource_group_header_list_type&& get_resource_group_headers() && noexcept
	{
		return std::move(resource_group_headers_);
	}

	[[nodiscard]]
	data_list_type& get_data_list() & noexcept
	{
		return data_list_;
	}

	[[nodiscard]]
	const data_list_type& get_data_list() const& noexcept
	{
		return data_list_;
	}

	[[nodiscard]]
	data_list_type&& get_data_list() && noexcept
	{
		return std::move(data_list_);
	}

private:
	header_type header_;
	resource_group_header_list_type resource_group_headers_;
	data_list_type data_list_;
};

using icon_group = icon_cursor_group<detail::resources::ico_header,
	detail::resources::icon_group, detail::resources::icon_type>;
using cursor_group = icon_cursor_group<detail::resources::cursor_header,
	detail::resources::cursor_group, detail::resources::cursor_type>;

} //namespace pe_bliss::resources
