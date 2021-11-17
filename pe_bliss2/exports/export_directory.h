#pragma once

#include <list>
#include <string_view>
#include <utility>

#include "pe_bliss2/detail/error_list.h"
#include "pe_bliss2/detail/packed_c_string.h"
#include "pe_bliss2/detail/packed_struct.h"
#include "pe_bliss2/detail/exports/image_export_directory.h"
#include "pe_bliss2/exports/exported_address.h"
#include "pe_bliss2/pe_types.h"

namespace pe_bliss
{
class image;
} //namespace pe_bliss

namespace pe_bliss::exports
{

template<typename ExportedAddressList>
class export_directory_base
{
public:
	using packed_descriptor_type = detail::packed_struct<detail::exports::image_export_directory>;
	using export_list_type = ExportedAddressList;
	using exported_address_type = export_list_type::value_type;

public:
	[[nodiscard]] packed_descriptor_type& get_descriptor() noexcept
	{
		return descriptor_;
	}

	[[nodiscard]] const packed_descriptor_type& get_descriptor() const noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	detail::packed_c_string& get_library_name() noexcept
	{
		return library_name_;
	}

	[[nodiscard]]
	const detail::packed_c_string& get_library_name() const noexcept
	{
		return library_name_;
	}

	[[nodiscard]]
	export_list_type& get_export_list() noexcept
	{
		return exported_addresses_;
	}

	[[nodiscard]]
	const export_list_type& get_export_list() const noexcept
	{
		return exported_addresses_;
	}

public:
	template<typename String>
	void set_library_name(String&& str) noexcept
	{
		library_name_ = std::forward<String>(str);
	}
	
public:
	[[nodiscard]]
	export_list_type::const_iterator symbol_by_ordinal(ordinal_type ordinal) const noexcept;
	[[nodiscard]]
	export_list_type::iterator symbol_by_ordinal(ordinal_type ordinal) noexcept;
	[[nodiscard]]
	export_list_type::const_iterator symbol_by_name(std::string_view name) const noexcept;
	[[nodiscard]]
	export_list_type::iterator symbol_by_name(std::string_view name) noexcept;

public:
	exported_address_type& add(ordinal_type rva_ordinal,
		std::string_view name, rva_type rva);
	exported_address_type& add(ordinal_type rva_ordinal, rva_type rva);
	exported_address_type& add(ordinal_type rva_ordinal,
		std::string_view name, std::string_view forwarded_name);

public:
	[[nodiscard]]
	ordinal_type get_first_free_ordinal() const;
	[[nodiscard]]
	ordinal_type get_last_free_ordinal() const;

private:
	packed_descriptor_type descriptor_;
	detail::packed_c_string library_name_;
	export_list_type exported_addresses_;
};

using export_directory = export_directory_base<std::list<exported_address>>;

class export_directory_details
	: public export_directory_base<std::list<exported_address_details>>
	, public detail::error_list
{
};

} //namespace pe_bliss::exports
