#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "buffers/ref_buffer.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/detail/resources/icon_cursor.h"

namespace pe_bliss::resources
{

template<typename Native>
class [[nodiscard]] icon_group_header_base
{
public:
	using header_type = packed_struct<Native>;

public:
	[[nodiscard]]
	const header_type& native() const noexcept
	{
		return header_;
	}

	[[nodiscard]]
	header_type& native() noexcept
	{
		return header_;
	}

	[[nodiscard]]
	const header_type& operator->() const noexcept
	{
		return header_;
	}

	[[nodiscard]]
	header_type& operator->() noexcept
	{
		return header_;
	}

	[[nodiscard]]
	std::uint16_t get_bit_count() const noexcept
	{
		return native()->bit_count;
	}

private:
	header_type header_;
};

class [[nodiscard]] icon_group_header final
	: public icon_group_header_base<detail::resources::icon_group>
{
public:
	[[nodiscard]]
	std::uint32_t get_height() const noexcept
	{
		auto result = native()->height;
		return result == 0u ? 256u : result;
	}

	[[nodiscard]]
	std::uint32_t get_width() const noexcept
	{
		auto result = native()->width;
		return result == 0u ? 256u : result;
	}
};

class [[nodiscard]] cursor_group_header final
	: public icon_group_header_base<detail::resources::cursor_group>
{
public:
	[[nodiscard]]
	std::uint32_t get_height() const noexcept
	{
		return native()->height / 2u;
	}

	[[nodiscard]]
	std::uint32_t get_width() const noexcept
	{
		return native()->width;
	}
};

template<typename Header, typename ResourceGroupHeader, std::uint16_t Type>
class [[nodiscard]] icon_cursor_group
{
public:
	static constexpr std::uint16_t type = Type;
	using header_type = packed_struct<Header>;
	using resource_group_header_type = ResourceGroupHeader;
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
	icon_group_header, detail::resources::icon_type>;
using cursor_group = icon_cursor_group<detail::resources::cursor_header,
	cursor_group_header, detail::resources::cursor_type>;

} //namespace pe_bliss::resources
