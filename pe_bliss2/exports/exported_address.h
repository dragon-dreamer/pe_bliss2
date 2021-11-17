#pragma once

#include <cstdint>
#include <list>
#include <optional>
#include <string>
#include <utility>

#include "pe_bliss2/detail/error_list.h"
#include "pe_bliss2/detail/packed_c_string.h"
#include "pe_bliss2/detail/packed_struct.h"
#include "pe_bliss2/pe_types.h"

namespace pe_bliss::exports
{

using ordinal_type = std::uint16_t;
using packed_ordinal_type = detail::packed_struct<ordinal_type>;
using packed_rva_type = detail::packed_struct<rva_type>;
using optional_c_string = std::optional<pe_bliss::detail::packed_c_string>;

class exported_name
{
public:
	exported_name() = default;

	template<typename String, typename NameRva, typename NameOrdinal>
	exported_name(String&& name, NameRva&& name_rva, NameOrdinal&& name_ordinal)
		: name_(std::forward<String>(name))
		, name_rva_(std::forward<NameRva>(name_rva))
		, name_ordinal_(std::forward<NameOrdinal>(name_ordinal))
	{
	}

	[[nodiscard]]
	optional_c_string& get_name() noexcept
	{
		return name_;
	}
	
	[[nodiscard]]
	const optional_c_string& get_name() const noexcept
	{
		return name_;
	}

	[[nodiscard]]
	packed_rva_type& get_name_rva() noexcept
	{
		return name_rva_;
	}

	[[nodiscard]]
	const packed_rva_type& get_name_rva() const noexcept
	{
		return name_rva_;
	}

	[[nodiscard]]
	packed_ordinal_type& get_name_ordinal() noexcept
	{
		return name_ordinal_;
	}

	[[nodiscard]]
	const packed_ordinal_type& get_name_ordinal() const noexcept
	{
		return name_ordinal_;
	}

private:
	optional_c_string name_;
	packed_rva_type name_rva_;
	packed_ordinal_type name_ordinal_{};
};

class exported_name_details : public exported_name, public detail::error_list
{
public:
	using exported_name::exported_name;
};

using exported_name_list = std::list<exported_name>;
using exported_name_details_list = std::list<exported_name_details>;

template<typename ExportedNameList>
class exported_address_base
{
public:
	using name_list_type = ExportedNameList;

public:
	[[nodiscard]]
	ordinal_type get_rva_ordinal() const noexcept
	{
		return rva_ordinal_;
	}

	void set_rva_ordinal(ordinal_type rva_ordinal) noexcept
	{
		rva_ordinal_ = rva_ordinal;
	}

	[[nodiscard]]
	detail::packed_struct<rva_type>& get_rva() noexcept
	{
		return rva_;
	}

	[[nodiscard]]
	const detail::packed_struct<rva_type>& get_rva() const noexcept
	{
		return rva_;
	}

	[[nodiscard]]
	name_list_type& get_names() noexcept
	{
		return names_;
	}

	[[nodiscard]]
	const name_list_type& get_names() const noexcept
	{
		return names_;
	}

	[[nodiscard]]
	optional_c_string& get_forwarded_name() noexcept
	{
		return forwarded_name_;
	}

	[[nodiscard]]
	const optional_c_string& get_forwarded_name() const noexcept
	{
		return forwarded_name_;
	}

private:
	ordinal_type rva_ordinal_{};
	detail::packed_struct<rva_type> rva_{};
	ExportedNameList names_;
	optional_c_string forwarded_name_;
};

using exported_address = exported_address_base<exported_name_list>;

class exported_address_details
	: public exported_address_base<exported_name_details_list>
	, public detail::error_list
{
};

struct forwarded_name_info
{
	std::string library_name;
	std::string function_name;
};

[[nodiscard]]
forwarded_name_info get_forwarded_name_info(const std::string& forwarded_name);

} //namespace pe_bliss::exports
