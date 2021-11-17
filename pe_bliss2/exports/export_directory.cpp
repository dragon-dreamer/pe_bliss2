#include "pe_bliss2/exports/export_directory.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <vector>

#include "pe_bliss2/pe_error.h"
#include "utilities/generic_error.h"

namespace
{

using namespace pe_bliss;
using namespace pe_bliss::exports;

template<typename ExportedAddressList>
auto symbol_by_ordinal_impl(ExportedAddressList& addresses,
	ordinal_type ordinal) noexcept
{
	return std::find_if(std::begin(addresses),
		std::end(addresses), [ordinal] (const auto& addr)
		{
			return addr.get_rva_ordinal() == ordinal;
		});
}

template<typename NameList>
bool has_name_impl(const NameList& names, std::string_view name) noexcept
{
	return std::find_if(std::begin(names),
		std::end(names), [name] (const auto& sym)
		{
			return sym.get_name() && sym.get_name()->value() == name;
		}) != std::end(names);
}

template<typename ExportedAddressList>
auto symbol_by_name_impl(ExportedAddressList& addresses,
	std::string_view name) noexcept
{
	return std::find_if(std::begin(addresses),
		std::end(addresses), [name] (const auto& addr)
		{
			return has_name_impl(addr.get_names(), name);
		});
}

} //namespace

namespace pe_bliss::exports
{

template<typename ExportedAddressList>
typename export_directory_base<ExportedAddressList>::export_list_type::const_iterator
	export_directory_base<ExportedAddressList>::symbol_by_ordinal(ordinal_type ordinal) const noexcept
{
	return symbol_by_ordinal_impl(exported_addresses_, ordinal);
}

template<typename ExportedAddressList>
typename export_directory_base<ExportedAddressList>::export_list_type::iterator
	export_directory_base<ExportedAddressList>::symbol_by_ordinal(ordinal_type ordinal) noexcept
{
	return symbol_by_ordinal_impl(exported_addresses_, ordinal);
}

template<typename ExportedAddressList>
typename export_directory_base<ExportedAddressList>::export_list_type::const_iterator
	export_directory_base<ExportedAddressList>::symbol_by_name(std::string_view name) const noexcept
{
	return symbol_by_name_impl(exported_addresses_, name);
}

template<typename ExportedAddressList>
typename export_directory_base<ExportedAddressList>::export_list_type::iterator
	export_directory_base<ExportedAddressList>::symbol_by_name(std::string_view name) noexcept
{
	return symbol_by_name_impl(exported_addresses_, name);
}

template<typename ExportedAddressList>
typename export_directory_base<ExportedAddressList>::exported_address_type&
	export_directory_base<ExportedAddressList>::add(ordinal_type rva_ordinal,
		std::string_view name, rva_type rva)
{
	auto& sym = add(rva_ordinal, rva);
	sym.get_names().emplace_back().get_name() = name;
	return sym;
}

template<typename ExportedAddressList>
typename export_directory_base<ExportedAddressList>::exported_address_type&
	export_directory_base<ExportedAddressList>::add(ordinal_type rva_ordinal, rva_type rva)
{
	auto& sym = exported_addresses_.emplace_back();
	sym.set_rva_ordinal(rva_ordinal);
	sym.get_rva() = rva;
	return sym;
}

template<typename ExportedAddressList>
typename export_directory_base<ExportedAddressList>::exported_address_type&
	export_directory_base<ExportedAddressList>::add(ordinal_type rva_ordinal,
		std::string_view name, std::string_view forwarded_name)
{
	auto& sym = add(rva_ordinal, name, 0u);
	sym.get_forwarded_name() = forwarded_name;
	return sym;
}

template<typename ExportedAddressList>
ordinal_type export_directory_base<ExportedAddressList>::get_first_free_ordinal() const
{
	if (exported_addresses_.empty())
		return {};

	std::vector<ordinal_type> occupied;
	occupied.reserve(exported_addresses_.size());
	for (const auto& addr : exported_addresses_)
		occupied.emplace_back(addr.get_rva_ordinal());

	std::sort(std::begin(occupied), std::end(occupied));

	ordinal_type next = {};
	for (ordinal_type ordinal : occupied)
	{
		if (ordinal != next)
			return next;
		next = ordinal + 1;
	}
	if (occupied.back() == (std::numeric_limits<ordinal_type>::max)())
		throw pe_error(utilities::generic_errc::integer_overflow);
	return occupied.back() + 1;
}

template<typename ExportedAddressList>
ordinal_type export_directory_base<ExportedAddressList>::get_last_free_ordinal() const
{
	auto it = std::max_element(std::cbegin(exported_addresses_), std::cend(exported_addresses_),
		[] (const auto& l, const auto& r)
		{
			return l.get_rva_ordinal() < r.get_rva_ordinal();
		});
	if (it == std::cend(exported_addresses_))
		return 0u;

	if (it->get_rva_ordinal() == (std::numeric_limits<ordinal_type>::max)())
		throw pe_error(utilities::generic_errc::integer_overflow);
	return it->get_rva_ordinal() + 1;
}

template class export_directory_base<std::list<exported_address>>;
template class export_directory_base<std::list<exported_address_details>>;

} //namespace pe_bliss::exports
