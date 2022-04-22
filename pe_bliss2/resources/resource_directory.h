#pragma once

#include <list>
#include <variant>

#include "buffers/ref_buffer.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/packed_utf16_string.h"
#include "pe_bliss2/detail/resources/image_resource_directory.h"
#include "pe_bliss2/pe_types.h"

namespace pe_bliss::resources
{

template<typename... Bases>
class resource_directory_entry_base;

template<typename... Bases>
class resource_directory_base : public Bases...
{
public:
	using packed_descriptor_type = packed_struct<detail::resources::image_resource_directory>;
	using entry_list_type = std::list<resource_directory_entry_base<Bases...>>;

public:
	[[nodiscard]]
	const packed_descriptor_type& get_descriptor() const noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	packed_descriptor_type& get_descriptor() noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	const entry_list_type& get_entries() const noexcept
	{
		return entries_;
	}

	[[nodiscard]]
	entry_list_type& get_entries() noexcept
	{
		return entries_;
	}

private:
	packed_descriptor_type descriptor_;
	entry_list_type entries_;
};

template<typename... Bases>
class resource_data_entry_base : public Bases...
{
public:
	using packed_descriptor_type = packed_struct<detail::resources::image_resource_data_entry>;

public:
	[[nodiscard]]
	const packed_descriptor_type& get_descriptor() const noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	packed_descriptor_type& get_descriptor() noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	const buffers::ref_buffer& get_raw_data() const noexcept
	{
		return raw_data_;
	}

	[[nodiscard]]
	buffers::ref_buffer& get_raw_data() noexcept
	{
		return raw_data_;
	}

private:
	packed_descriptor_type descriptor_;
	buffers::ref_buffer raw_data_;
};

using resource_id_type = std::uint32_t;

template<typename... Bases>
class resource_directory_entry_base : public Bases...
{
public:
	using packed_descriptor_type = packed_struct<detail::resources::image_resource_directory_entry>;
	using name_or_id_type = std::variant<std::monostate,
		resource_id_type, packed_utf16_string>;
	using data_or_directory_type = std::variant<std::monostate,
		resource_directory_base<Bases...>, resource_data_entry_base<Bases...>,
		rva_type /* RVA of looped resource_directory_base */>;

public:
	[[nodiscard]]
	const packed_descriptor_type& get_descriptor() const noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	packed_descriptor_type& get_descriptor() noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	const name_or_id_type& get_name_or_id() const noexcept
	{
		return name_or_id_;
	}

	[[nodiscard]]
	name_or_id_type& get_name_or_id() noexcept
	{
		return name_or_id_;
	}

	[[nodiscard]]
	bool is_named() const noexcept
	{
		static constexpr std::size_t name_index = 2u;
		return name_or_id_.index() == name_index;
	}

	[[nodiscard]]
	const data_or_directory_type& get_data_or_directory() const noexcept
	{
		return data_or_directory_;
	}

	[[nodiscard]]
	data_or_directory_type& get_data_or_directory() noexcept
	{
		return data_or_directory_;
	}

private:
	packed_descriptor_type descriptor_;
	name_or_id_type name_or_id_;
	data_or_directory_type data_or_directory_;
};

using resource_directory_entry = resource_directory_entry_base<>;
using resource_directory_entry_details = resource_directory_entry_base<error_list>;
using resource_data_entry = resource_data_entry_base<>;
using resource_data_entry_details = resource_data_entry_base<error_list>;
using resource_directory = resource_directory_base<>;
using resource_directory_details = resource_directory_base<error_list>;

} //namespace pe_bliss::resources
