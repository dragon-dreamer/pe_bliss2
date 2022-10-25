#pragma once

#include <cstdint>
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
//TODO: IMAGE_DELAYLOAD_DESCRIPTOR is different from loader imports

template<typename ImportedAddressList>
class [[nodiscard]] imported_library_base
	: public detail::packed_struct_base<detail::imports::image_import_descriptor>
{
public:
	using imported_address_list = ImportedAddressList;

public:
	[[nodiscard]]
	bool is_bound() const noexcept
	{
		return descriptor_->time_date_stamp == 0xffffffffu;
	}

	[[nodiscard]]
	bool has_lookup_table() const noexcept
	{
		return descriptor_->lookup_table
			&& descriptor_->lookup_table != descriptor_->address_table;
	}

public:
	void set_bound() noexcept
	{
		descriptor_->time_date_stamp = 0xffffffffu;
	}

	void remove_lookup_table() noexcept
	{
		descriptor_->lookup_table = 0u;
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

template<detail::executable_pointer Va>
class [[nodiscard]] imported_library
	: public imported_library_base<imported_address_list<Va>>
{
};

template<detail::executable_pointer Va>
class [[nodiscard]] imported_library_details
	: public imported_library_base<imported_address_details_list<Va>>
	, public error_list
{
};

template<template <detail::executable_pointer> typename ImportedLibrary>
class [[nodiscard]] import_directory_base
{
public:
	using imported_library_list_type = std::variant<
		std::vector<ImportedLibrary<std::uint32_t>>,
		std::vector<ImportedLibrary<std::uint64_t>>
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

using import_directory = import_directory_base<imported_library>;
class [[nodiscard]] import_directory_details
	: public import_directory_base<imported_library_details>
	, public error_list
{
};

} //namespace pe_bliss::imports
