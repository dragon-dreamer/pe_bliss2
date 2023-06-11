#pragma once

#include <string_view>
#include <utility>
#include <vector>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/packed_c_string.h"
#include "pe_bliss2/detail/exports/image_export_directory.h"
#include "pe_bliss2/detail/packed_struct_base.h"
#include "pe_bliss2/exports/exported_address.h"
#include "pe_bliss2/pe_types.h"

namespace pe_bliss::exports
{

template<typename ExportedAddressList>
class [[nodiscard]] export_directory_base
	: public detail::packed_struct_base<detail::exports::image_export_directory>
{
public:
	using export_list_type = ExportedAddressList;
	using exported_address_type = typename export_list_type::value_type;

public:
	[[nodiscard]]
	packed_c_string& get_library_name() & noexcept
	{
		return library_name_;
	}

	[[nodiscard]]
	packed_c_string get_library_name() && noexcept
	{
		return std::move(library_name_);
	}

	[[nodiscard]]
	const packed_c_string& get_library_name() const & noexcept
	{
		return library_name_;
	}

	[[nodiscard]]
	export_list_type& get_export_list() & noexcept
	{
		return exported_addresses_;
	}

	[[nodiscard]]
	export_list_type get_export_list() && noexcept
	{
		return std::move(exported_addresses_);
	}

	[[nodiscard]]
	const export_list_type& get_export_list() const & noexcept
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
	typename export_list_type::const_iterator symbol_by_ordinal(ordinal_type rva_ordinal) const noexcept;
	[[nodiscard]]
	typename export_list_type::iterator symbol_by_ordinal(ordinal_type rva_ordinal) noexcept;
	[[nodiscard]]
	typename export_list_type::const_iterator symbol_by_name(std::string_view name) const noexcept;
	[[nodiscard]]
	typename export_list_type::iterator symbol_by_name(std::string_view name) noexcept;

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
	packed_c_string library_name_;
	export_list_type exported_addresses_;
};

using export_directory = export_directory_base<std::vector<exported_address>>;

class [[nodiscard]] export_directory_details
	: public export_directory_base<std::vector<exported_address_details>>
	, public error_list
{
};

} //namespace pe_bliss::exports
