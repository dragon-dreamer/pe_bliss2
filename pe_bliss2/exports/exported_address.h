#pragma once

#include <cstdint>
#include <compare>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/packed_c_string.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/pe_types.h"

namespace pe_bliss::exports
{

using ordinal_type = std::uint16_t;
using packed_ordinal_type = packed_struct<ordinal_type>;
using packed_rva_type = packed_struct<rva_type>;
using optional_c_string = std::optional<packed_c_string>;

class [[nodiscard]] exported_name
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
	optional_c_string& get_name() & noexcept
	{
		return name_;
	}

	[[nodiscard]]
	optional_c_string get_name() && noexcept
	{
		return std::move(name_);
	}
	
	[[nodiscard]]
	const optional_c_string& get_name() const & noexcept
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

class [[nodiscard]] exported_name_details : public exported_name, public error_list
{
public:
	using exported_name::exported_name;
};

using exported_name_list = std::vector<exported_name>;
using exported_name_details_list = std::vector<exported_name_details>;

template<typename ExportedNameList>
class [[nodiscard]] exported_address_base
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
	packed_struct<rva_type>& get_rva() noexcept
	{
		return rva_;
	}

	[[nodiscard]]
	const packed_struct<rva_type>& get_rva() const noexcept
	{
		return rva_;
	}

	[[nodiscard]]
	name_list_type& get_names() & noexcept
	{
		return names_;
	}

	[[nodiscard]]
	const name_list_type& get_names() const & noexcept
	{
		return names_;
	}

	[[nodiscard]]
	name_list_type get_names() && noexcept
	{
		return std::move(names_);
	}

	[[nodiscard]]
	optional_c_string& get_forwarded_name() & noexcept
	{
		return forwarded_name_;
	}

	[[nodiscard]]
	const optional_c_string& get_forwarded_name() const & noexcept
	{
		return forwarded_name_;
	}

	[[nodiscard]]
	optional_c_string get_forwarded_name() && noexcept
	{
		return std::move(forwarded_name_);
	}

private:
	ordinal_type rva_ordinal_{};
	packed_struct<rva_type> rva_{};
	ExportedNameList names_;
	optional_c_string forwarded_name_;
};

using exported_address = exported_address_base<exported_name_list>;

class [[nodiscard]] exported_address_details
	: public exported_address_base<exported_name_details_list>
	, public error_list
{
};

struct [[nodiscard]] forwarded_name_info
{
	std::string library_name;
	std::string function_name;

	[[nodiscard]]
	friend auto operator<=>(const forwarded_name_info&,
		const forwarded_name_info&) = default;
	[[nodiscard]]
	friend bool operator==(const forwarded_name_info&,
		const forwarded_name_info&) = default;
};

[[nodiscard]]
forwarded_name_info get_forwarded_name_info(const std::string& forwarded_name);

} //namespace pe_bliss::exports
