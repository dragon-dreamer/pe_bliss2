#include "pe_bliss2/resources/resource_directory.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/packed_utf16_string.h"
#include "pe_bliss2/resources/resource_types.h"

namespace
{
using namespace pe_bliss::resources;

struct resource_directory_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "resource_directory";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::resources::resource_directory_errc;
		switch (static_cast<pe_bliss::resources::resource_directory_errc>(ev))
		{
		case entry_does_not_exist:
			return "Resource directory entry does not exist";
		case entry_does_not_contain_directory:
			return "Resource directory entry does not contain resource directory";
		case entry_does_not_contain_data:
			return "Resource directory entry does not contain resource data entry";
		case entry_does_not_have_name:
			return "Resource directory entry does not have name";
		case entry_does_not_have_id:
			return "Resource directory entry does not have ID";
		default:
			return {};
		}
	}
};

const resource_directory_error_category resource_directory_error_category_instance;

struct id_entry_finder
{
public:
	explicit id_entry_finder(resource_id_type id) noexcept
		: id_(id)
	{
	}

	template<typename Entry>
	bool operator()(const Entry& entry) noexcept
	{
		const auto* id = std::get_if<resource_id_type>(&entry.get_name_or_id());
		return id && *id == id_;
	}

private:
	resource_id_type id_;
};

struct name_entry_finder
{
public:
	explicit name_entry_finder(std::u16string_view name) noexcept
		: name_(name)
	{
	}

	template<typename Entry>
	bool operator()(const Entry& entry) noexcept
	{
		const auto* name = std::get_if<pe_bliss::packed_utf16_string>(
			&entry.get_name_or_id());
		return name && name->value() == name_;
	}

private:
	std::u16string_view name_;
};

} //namespace

namespace pe_bliss::resources
{

std::error_code make_error_code(resource_directory_errc e) noexcept
{
	return { static_cast<int>(e), resource_directory_error_category_instance };
}

template<typename... Bases>
typename resource_directory_base<Bases...>::entry_list_type::const_iterator
resource_directory_base<Bases...>::entry_iterator_by_id(resource_id_type id) const noexcept
{
	return std::find_if(entries_.begin(), entries_.end(), id_entry_finder(id));
}

template<typename... Bases>
typename resource_directory_base<Bases...>::entry_list_type::iterator
resource_directory_base<Bases...>::entry_iterator_by_id(resource_id_type id) noexcept
{
	return std::find_if(entries_.begin(), entries_.end(), id_entry_finder(id));
}

template<typename... Bases>
typename resource_directory_base<Bases...>::entry_list_type::const_iterator
resource_directory_base<Bases...>::entry_iterator_by_name(std::u16string_view name) const noexcept
{
	return std::find_if(entries_.begin(), entries_.end(), name_entry_finder(name));
}

template<typename... Bases>
typename resource_directory_base<Bases...>::entry_list_type::iterator
resource_directory_base<Bases...>::entry_iterator_by_name(std::u16string_view name) noexcept
{
	return std::find_if(entries_.begin(), entries_.end(), name_entry_finder(name));
}

template<typename... Bases>
const typename resource_directory_base<Bases...>::entry_type&
resource_directory_base<Bases...>::entry_by_id(resource_id_type id) const
{
	auto it = entry_iterator_by_id(id);
	if (it == entries_.cend())
		throw pe_error(resource_directory_errc::entry_does_not_exist);

	return *it;
}

template<typename... Bases>
typename resource_directory_base<Bases...>::entry_type&
resource_directory_base<Bases...>::entry_by_id(resource_id_type id)
{
	auto it = entry_iterator_by_id(id);
	if (it == entries_.end())
		throw pe_error(resource_directory_errc::entry_does_not_exist);

	return *it;
}

template<typename... Bases>
const typename resource_directory_base<Bases...>::entry_type*
resource_directory_base<Bases...>::try_entry_by_id(resource_id_type id) const noexcept
{
	auto it = entry_iterator_by_id(id);
	return it == entries_.cend() ? nullptr : &*it;
}

template<typename... Bases>
typename resource_directory_base<Bases...>::entry_type*
resource_directory_base<Bases...>::try_entry_by_id(resource_id_type id) noexcept
{
	auto it = entry_iterator_by_id(id);
	return it == entries_.end() ? nullptr : &*it;
}

template<typename... Bases>
const typename resource_directory_base<Bases...>::entry_type&
resource_directory_base<Bases...>::entry_by_name(std::u16string_view name) const
{
	auto it = entry_iterator_by_name(name);
	if (it == entries_.cend())
		throw pe_error(resource_directory_errc::entry_does_not_exist);

	return *it;
}

template<typename... Bases>
typename resource_directory_base<Bases...>::entry_type&
resource_directory_base<Bases...>::entry_by_name(std::u16string_view name)
{
	auto it = entry_iterator_by_name(name);
	if (it == entries_.end())
		throw pe_error(resource_directory_errc::entry_does_not_exist);

	return *it;
}

template<typename... Bases>
const typename resource_directory_base<Bases...>::entry_type*
resource_directory_base<Bases...>::try_entry_by_name(std::u16string_view name) const noexcept
{
	auto it = entry_iterator_by_name(name);
	return it == entries_.cend() ? nullptr : &*it;
}

template<typename... Bases>
typename resource_directory_base<Bases...>::entry_type*
resource_directory_base<Bases...>::try_entry_by_name(std::u16string_view name) noexcept
{
	auto it = entry_iterator_by_name(name);
	return it == entries_.end() ? nullptr : &*it;
}

template<typename... Bases>
resource_directory_base<Bases...>& resource_directory_entry_base<Bases...>::get_directory()
{
	auto* dir = std::get_if<resource_directory_base<Bases...>>(&data_or_directory_);
	if (!dir)
		throw pe_error(resource_directory_errc::entry_does_not_contain_directory);

	return *dir;
}

template<typename... Bases>
resource_data_entry_base<Bases...>& resource_directory_entry_base<Bases...>::get_data()
{
	auto* data = std::get_if<resource_data_entry_base<Bases...>>(&data_or_directory_);
	if (!data)
		throw pe_error(resource_directory_errc::entry_does_not_contain_data);

	return *data;
}

template<typename... Bases>
const resource_directory_base<Bases...>& resource_directory_entry_base<Bases...>::get_directory() const
{
	const auto* dir = std::get_if<resource_directory_base<Bases...>>(&data_or_directory_);
	if (!dir)
		throw pe_error(resource_directory_errc::entry_does_not_contain_directory);

	return *dir;
}

template<typename... Bases>
const resource_data_entry_base<Bases...>& resource_directory_entry_base<Bases...>::get_data() const
{
	const auto* data = std::get_if<resource_data_entry_base<Bases...>>(&data_or_directory_);
	if (!data)
		throw pe_error(resource_directory_errc::entry_does_not_contain_data);

	return *data;
}

template<typename... Bases>
resource_name_type& resource_directory_entry_base<Bases...>::get_name()
{
	auto* name = std::get_if<resource_name_type>(&name_or_id_);
	if (!name)
		throw pe_error(resource_directory_errc::entry_does_not_have_name);

	return *name;
}

template<typename... Bases>
resource_id_type& resource_directory_entry_base<Bases...>::get_id()
{
	auto* id = std::get_if<resource_id_type>(&name_or_id_);
	if (!id)
		throw pe_error(resource_directory_errc::entry_does_not_have_id);

	return *id;
}

template<typename... Bases>
const resource_name_type& resource_directory_entry_base<Bases...>::get_name() const
{
	const auto* name = std::get_if<resource_name_type>(&name_or_id_);
	if (!name)
		throw pe_error(resource_directory_errc::entry_does_not_have_name);

	return *name;
}

template<typename... Bases>
const resource_id_type& resource_directory_entry_base<Bases...>::get_id() const
{
	const auto* id = std::get_if<resource_id_type>(&name_or_id_);
	if (!id)
		throw pe_error(resource_directory_errc::entry_does_not_have_id);

	return *id;
}

namespace
{
template<typename NameOrId, typename... Bases>
typename resource_directory_base<Bases...>::entry_type& try_emplace_entry(
	NameOrId&& name_or_id, directory_entry_contents contents,
	resource_directory_base<Bases...>& dir)
{
	typename resource_directory_base<Bases...>::entry_list_type::iterator it;
	static constexpr bool is_id = std::is_same_v<
		std::remove_cvref_t<NameOrId>, resource_id_type>;
	if constexpr (is_id)
		it = dir.entry_iterator_by_id(name_or_id);
	else
		it = dir.entry_iterator_by_name(name_or_id);

	if (it == dir.get_entries().end())
	{
		auto& entry = dir.get_entries().emplace_back();
		if constexpr (is_id)
		{
			entry.get_name_or_id() = name_or_id;
		}
		else
		{
			entry.get_name_or_id() = packed_utf16_string(
				std::forward<NameOrId>(name_or_id));
		}

		it = std::prev(dir.get_entries().end());
	}

	auto& data_or_directory = it->get_data_or_directory();
	if (std::holds_alternative<std::monostate>(data_or_directory))
	{
		if (contents == directory_entry_contents::data)
			data_or_directory.template emplace<resource_data_entry_base<Bases...>>();
		else
			data_or_directory.template emplace<resource_directory_base<Bases...>>();
	}
	else
	{
		if (contents == directory_entry_contents::data)
		{
			if (!std::holds_alternative<resource_data_entry_base<
				Bases...>>(data_or_directory))
			{
				throw pe_error(resource_directory_errc::entry_does_not_contain_data);
			}
		}
		else
		{
			if (!std::holds_alternative<resource_directory_base<
				Bases...>>(data_or_directory))
			{
				throw pe_error(resource_directory_errc::entry_does_not_contain_directory);
			}
		}
	}

	return *it;
}
} //namespace

template<typename... Bases>
typename resource_directory_base<Bases...>::entry_type& resource_directory_base<Bases...>
	::try_emplace_entry_by_id(resource_id_type id,
		directory_entry_contents contents)
{
	return try_emplace_entry(id, contents, *this);
}

template<typename... Bases>
typename resource_directory_base<Bases...>::entry_type& resource_directory_base<Bases...>
	::try_emplace_entry_by_name(std::u16string_view name,
		directory_entry_contents contents)
{
	return try_emplace_entry(name, contents, *this);
}

template<typename... Bases>
typename resource_directory_base<Bases...>::entry_type& resource_directory_base<Bases...>
::try_emplace_entry_by_name(directory_entry_contents contents,
	std::u16string&& name)
{
	return try_emplace_entry(std::move(name), contents, *this);
}

template class resource_directory_entry_base<>;
template class resource_directory_entry_base<error_list>;
template class resource_directory_base<>;
template class resource_directory_base<error_list>;

} //namespace pe_bliss::resources
