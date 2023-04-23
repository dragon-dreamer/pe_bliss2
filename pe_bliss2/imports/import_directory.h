#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/packed_c_string.h"
#include "pe_bliss2/detail/imports/image_import_descriptor.h"
#include "pe_bliss2/detail/packed_struct_base.h"
#include "pe_bliss2/imports/imported_address.h"

namespace pe_bliss::imports
{

//TODO: apisetschema.dll redirects parser

template<typename ImportedAddressList, typename Descriptor>
class [[nodiscard]] imported_library_base
	: public detail::packed_struct_base<Descriptor>
{
public:
	using imported_address_list = ImportedAddressList;

	static constexpr bool is_delayload = !std::is_same_v<
		Descriptor, detail::imports::image_import_descriptor>;

public:
	[[nodiscard]]
	bool is_rva_based() const noexcept requires(is_delayload)
	{
		return static_cast<bool>(this->get_descriptor()->all_attributes & 0x1u);
	}

public:
	[[nodiscard]]
	bool is_bound() const noexcept requires(!is_delayload)
	{
		return this->get_descriptor()->time_date_stamp == 0xffffffffu;
	}

	[[nodiscard]]
	bool is_bound() const noexcept requires(is_delayload)
	{
		return this->get_descriptor()->time_date_stamp != 0u;
	}

	[[nodiscard]]
	bool has_lookup_table() const noexcept
	{
		return this->get_descriptor()->lookup_table
			&& this->get_descriptor()->lookup_table
				!= this->get_descriptor()->address_table;
	}

public:
	void set_bound() noexcept requires(!is_delayload)
	{
		this->get_descriptor()->time_date_stamp = 0xffffffffu;
	}

	void remove_lookup_table() noexcept requires(!is_delayload)
	{
		this->get_descriptor()->lookup_table = 0u;
	}

public:
	[[nodiscard]]
	const packed_c_string& get_library_name() const & noexcept
	{
		return library_name_;
	}

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
	const imported_address_list& get_imports() const & noexcept
	{
		return imports_;
	}

	[[nodiscard]]
	imported_address_list& get_imports() & noexcept
	{
		return imports_;
	}

	[[nodiscard]]
	imported_address_list get_imports() && noexcept
	{
		return std::move(imports_);
	}

public:
	packed_c_string library_name_;
	imported_address_list imports_;
};

template<detail::executable_pointer Va, typename Descriptor>
class [[nodiscard]] imported_library
	: public imported_library_base<imported_address_list<Va, !std::is_same_v<
		Descriptor, detail::imports::image_import_descriptor>>, Descriptor>
{
};

template<detail::executable_pointer Va, typename Descriptor>
class [[nodiscard]] imported_library_details
	: public imported_library_base<imported_address_details_list<Va, !std::is_same_v<
		Descriptor, detail::imports::image_import_descriptor>>, Descriptor>
	, public error_list
{
};

template<template <detail::executable_pointer,
	typename Descriptor> typename ImportedLibrary, typename Descriptor>
class [[nodiscard]] import_directory_base
{
public:
	using imported_library32_type = ImportedLibrary<std::uint32_t, Descriptor>;
	using imported_library64_type = ImportedLibrary<std::uint64_t, Descriptor>;

public:
	using imported_library_list_type = std::variant<
		std::vector<imported_library32_type>,
		std::vector<imported_library64_type>
	>;

public:
	[[nodiscard]] imported_library_list_type& get_list() & noexcept
	{
		return list_;
	}

	[[nodiscard]] const imported_library_list_type& get_list() const & noexcept
	{
		return list_;
	}

	[[nodiscard]] imported_library_list_type get_list() && noexcept
	{
		return std::move(list_);
	}

private:
	imported_library_list_type list_;
};

using import_directory = import_directory_base<imported_library,
	detail::imports::image_import_descriptor>;
class [[nodiscard]] import_directory_details
	: public import_directory_base<imported_library_details,
		detail::imports::image_import_descriptor>
	, public error_list
{
};

} //namespace pe_bliss::imports
