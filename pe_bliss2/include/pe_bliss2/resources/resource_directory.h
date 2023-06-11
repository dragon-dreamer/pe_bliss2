#pragma once

#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "buffers/ref_buffer.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/detail/packed_struct_base.h"
#include "pe_bliss2/detail/resources/image_resource_directory.h"
#include "pe_bliss2/pe_types.h"
#include "pe_bliss2/resources/resource_types.h"

namespace pe_bliss::resources
{

enum class resource_directory_errc
{
	entry_does_not_exist = 1,
	entry_does_not_contain_directory,
	entry_does_not_contain_data,
	entry_does_not_have_name,
	entry_does_not_have_id
};

std::error_code make_error_code(resource_directory_errc) noexcept;

template<typename... Bases>
class resource_directory_entry_base;

enum class directory_entry_contents
{
	directory,
	data
};

template<typename... Bases>
class [[nodiscard]] resource_directory_base
	: public detail::packed_struct_base<detail::resources::image_resource_directory>
	, public Bases...
{
public:
	using entry_type = resource_directory_entry_base<Bases...>;
	using entry_list_type = std::vector<entry_type>;

public:
	[[nodiscard]]
	const entry_list_type& get_entries() const& noexcept
	{
		return entries_;
	}

	[[nodiscard]]
	entry_list_type& get_entries() & noexcept
	{
		return entries_;
	}

	[[nodiscard]]
	entry_list_type get_entries() && noexcept
	{
		return std::move(entries_);
	}

	[[nodiscard]]
	const entry_type& entry_by_id(resource_id_type id) const;
	[[nodiscard]]
	entry_type& entry_by_id(resource_id_type id);
	[[nodiscard]]
	const entry_type* try_entry_by_id(resource_id_type id) const noexcept;
	[[nodiscard]]
	entry_type* try_entry_by_id(resource_id_type id) noexcept;
	[[nodiscard]]
	const entry_type& entry_by_name(std::u16string_view name) const;
	[[nodiscard]]
	entry_type& entry_by_name(std::u16string_view name);
	[[nodiscard]]
	const entry_type* try_entry_by_name(std::u16string_view name) const noexcept;
	[[nodiscard]]
	entry_type* try_entry_by_name(std::u16string_view name) noexcept;

	[[nodiscard]]
	typename entry_list_type::const_iterator entry_iterator_by_id(resource_id_type id) const noexcept;
	[[nodiscard]]
	typename entry_list_type::iterator entry_iterator_by_id(resource_id_type id) noexcept;
	[[nodiscard]]
	typename entry_list_type::const_iterator entry_iterator_by_name(std::u16string_view name) const noexcept;
	[[nodiscard]]
	typename entry_list_type::iterator entry_iterator_by_name(std::u16string_view name) noexcept;

	entry_type& try_emplace_entry_by_id(resource_id_type id,
		directory_entry_contents contents);
	entry_type& try_emplace_entry_by_name(std::u16string_view name,
		directory_entry_contents contents);
	entry_type& try_emplace_entry_by_name(directory_entry_contents contents,
		std::u16string&& name);

private:
	entry_list_type entries_;
};

template<typename... Bases>
class [[nodiscard]] resource_data_entry_base
	: public detail::packed_struct_base<detail::resources::image_resource_data_entry>
	, public Bases...
{
public:
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
	buffers::ref_buffer raw_data_;
};

template<typename... Bases>
class [[nodiscard]] resource_directory_entry_base
	: public detail::packed_struct_base<detail::resources::image_resource_directory_entry>
	, public Bases...
{
public:
	using name_or_id_type = std::variant<std::monostate,
		resource_id_type, resource_name_type>;
	using data_or_directory_type = std::variant<std::monostate,
		resource_directory_base<Bases...>, resource_data_entry_base<Bases...>,
		rva_type /* RVA of looped resource_directory_base */>;

public:
	[[nodiscard]]
	const name_or_id_type& get_name_or_id() const& noexcept
	{
		return name_or_id_;
	}

	[[nodiscard]]
	name_or_id_type& get_name_or_id() & noexcept
	{
		return name_or_id_;
	}

	[[nodiscard]]
	name_or_id_type get_name_or_id() && noexcept
	{
		return std::move(name_or_id_);
	}

	[[nodiscard]]
	bool is_named() const noexcept
	{
		static constexpr std::size_t name_index = 2u;
		return name_or_id_.index() == name_index;
	}

	[[nodiscard]]
	bool has_id() const noexcept
	{
		static constexpr std::size_t id_index = 1u;
		return name_or_id_.index() == id_index;
	}

	[[nodiscard]]
	resource_name_type& get_name();
	[[nodiscard]]
	resource_id_type& get_id();

	[[nodiscard]]
	const resource_name_type& get_name() const;
	[[nodiscard]]
	const resource_id_type& get_id() const;

	[[nodiscard]]
	const data_or_directory_type& get_data_or_directory() const& noexcept
	{
		return data_or_directory_;
	}

	[[nodiscard]]
	data_or_directory_type& get_data_or_directory() & noexcept
	{
		return data_or_directory_;
	}

	[[nodiscard]]
	data_or_directory_type get_data_or_directory() && noexcept
	{
		return std::move(data_or_directory_);
	}

	[[nodiscard]]
	bool has_data() const noexcept
	{
		static constexpr std::size_t data_index = 2u;
		return data_or_directory_.index() == data_index;
	}

	[[nodiscard]]
	bool has_directory() const noexcept
	{
		static constexpr std::size_t data_index = 1u;
		return data_or_directory_.index() == data_index;
	}

	[[nodiscard]]
	resource_directory_base<Bases...>& get_directory();
	[[nodiscard]]
	resource_data_entry_base<Bases...>& get_data();

	[[nodiscard]]
	const resource_directory_base<Bases...>& get_directory() const;
	[[nodiscard]]
	const resource_data_entry_base<Bases...>& get_data() const;

private:
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

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::resources::resource_directory_errc> : true_type {};
} //namespace std
