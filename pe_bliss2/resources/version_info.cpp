#include "pe_bliss2/resources/version_info.h"

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <system_error>
#include <tuple>
#include <variant>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "buffers/ref_buffer.h"
#include "pe_bliss2/pe_error.h"

namespace
{
constexpr pe_bliss::resources::translation neutral{ 0x0000u, 0x04b0u };
constexpr pe_bliss::resources::translation neutral_process_default{ 0x0400u, 0x04b0u };
constexpr pe_bliss::resources::translation neutral_system_default{ 0x0800u, 0x04b0u };
constexpr pe_bliss::resources::translation english_neutral{ 0x0009u, 0x04b0u };
constexpr pe_bliss::resources::translation english_us{ 0x0409u, 0x04b0u };

constexpr std::u16string_view version_info_key(u"VS_VERSION_INFO");
constexpr std::u16string_view string_file_info_key(u"StringFileInfo");
constexpr std::u16string_view var_file_info_key(u"VarFileInfo");
constexpr std::u16string_view translation_key(u"Translation");

struct version_info_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "version_info";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::resources::version_info_errc;
		switch (static_cast<pe_bliss::resources::version_info_errc>(ev))
		{
		case language_does_not_exist:
			return "Language does not exist";
		case key_does_not_exist:
			return "Key does not exist";
		case incorrect_root_block_key:
			return "Incorrect root version info block key";
		case absent_file_version_info:
			return "Absent file version info";
		case file_version_info_read_error:
			return "File version info buffer read error";
		case unknown_version_info_key:
			return "Unknown version info key";
		case invalid_var_file_info_children:
			return "Invalid var file info children";
		case invalid_translations:
			return "Invalid translations";
		case absent_translations:
			return "Absent translations";
		case duplicate_translations:
			return "Duplicate translations";
		case invalid_string_translations:
			return "Invalid string translations";
		case duplicate_string_translations:
			return "Duplicate string translations";
		case invalid_strings:
			return "Invalid strings";
		case duplicate_strings:
			return "Duplicate strings";
		case duplicate_string_file_info:
			return "Duplicate StringFileInfo key";
		case duplicate_var_file_info:
			return "Duplicate VarFileInfo key";
		default:
			return {};
		}
	}
};

const version_info_error_category version_info_error_category_instance;
} //namespace

namespace pe_bliss::resources
{

std::error_code make_error_code(version_info_errc e) noexcept
{
	return { static_cast<int>(e), version_info_error_category_instance };
}

std::optional<translation> parse_translation(std::u16string_view text)
{
	static constexpr std::size_t lcid_length = 4u;
	static constexpr std::size_t cpid_length = 4u;
	static constexpr int lcid_cpid_base = 16;

	std::array<char, lcid_length + cpid_length> translation_str{};
	if (text.size() != translation_str.size())
		return {};

	auto translation_str_it = translation_str.begin();
	for (char16_t ch : text)
	{
		if (ch > (std::numeric_limits<std::uint8_t>::max)())
			return {};
		*translation_str_it++ = static_cast<char>(ch);
	}

	translation result{};
	if (std::from_chars(translation_str.data(),
		translation_str.data() + lcid_length,
		result.lcid, lcid_cpid_base)
		!= std::from_chars_result{ translation_str.data() + lcid_length }
		|| std::from_chars(translation_str.data() + lcid_length,
			translation_str.data() + lcid_length + cpid_length,
			result.cpid, lcid_cpid_base)
			!= std::from_chars_result{ translation_str.data() + lcid_length + cpid_length })
	{
		return {};
	}

	return result;
}

template<typename ChildList>
void load_strings(version_info_details& result,
	string_value_map_type& strings, const ChildList& children)
{
	for (const auto& child : children)
	{
		if (!child.get_key())
		{
			result.add_error(version_info_errc::invalid_strings);
			continue;
		}

		const auto* str = std::get_if<packed_utf16_c_string>(&child.get_value());
		if (!str)
		{
			result.add_error(version_info_errc::invalid_strings);
			continue;
		}

		if (!strings.emplace(child.get_key()->value(), str->value()).second)
			result.add_error(version_info_errc::duplicate_strings);
	}
}

template<typename ChildList>
void load_strings(version_info_details& result, const ChildList& children)
{
	for (const auto& child : children)
	{
		if (!child.get_key())
		{
			result.add_error(version_info_errc::invalid_string_translations);
			continue;
		}

		auto translation_value = parse_translation(child.get_key()->value());
		if (!translation_value)
		{
			result.add_error(version_info_errc::invalid_string_translations);
			continue;
		}
		
		const auto [it, inserted] = result.get_strings().emplace(
			std::piecewise_construct,
			std::forward_as_tuple(*translation_value),
			std::forward_as_tuple());
		if (!inserted)
		{
			result.add_error(version_info_errc::duplicate_string_translations);
			continue;
		}

		load_strings(result, it->second, child.get_children());
	}
}

template<typename ChildList>
void load_var_file_info(version_info_details& result,
	const ChildList& children, bool allow_virtual_data)
{
	if (children.size() != 1u)
		result.add_error(version_info_errc::invalid_var_file_info_children);

	for (const auto& child : children)
	{
		if (child.get_key() && child.get_key()->value() == translation_key)
		{
			const auto* translations = std::get_if<buffers::ref_buffer>(&child.get_value());
			if (!translations)
			{
				result.add_error(version_info_errc::invalid_translations);
				continue;
			}

			buffers::input_buffer_stateful_wrapper_ref ref(*translations->data());
			packed_struct<detail::resources::translation_block> translation_data;

			try
			{
				while (ref.rpos() < ref.size())
				{
					translation_data.get() = {};
					translation_data.deserialize(ref, allow_virtual_data);
					if (!result.get_translations().emplace(translation{
						.lcid = translation_data->lcid, .cpid = translation_data->cpid }).second)
					{
						result.add_error(version_info_errc::duplicate_translations);
					}
				}
			}
			catch (const std::system_error&)
			{
				result.add_error(version_info_errc::invalid_translations);
			}

			return;
		}
	}
	result.add_error(version_info_errc::absent_translations);
}

template<typename... Bases>
version_info_details get_version_info(const version_info_block_base<Bases...>& root,
	const version_info_load_options<Bases...>& options)
{
	version_info_details result;
	if (!root.get_key() || root.get_key()->value() != version_info_key)
	{
		result.add_error(version_info_errc::incorrect_root_block_key);
		return result;
	}

	const auto* fixed_info = std::get_if<buffers::ref_buffer>(&root.get_value());
	if (!fixed_info)
	{
		result.add_error(version_info_errc::absent_file_version_info);
	}
	else
	{
		try
		{
			buffers::input_buffer_stateful_wrapper_ref ref(*fixed_info->data());
			result.get_file_version_info().get_descriptor().deserialize(ref,
				options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			result.add_error(version_info_errc::file_version_info_read_error);
		}
	}

	bool has_strings = false, has_translations = false;
	for (const auto& child : root.get_children())
	{
		const auto& key = child.get_key();
		if (!key)
		{
			result.add_error(version_info_errc::unknown_version_info_key);
			continue;
		}

		if (key->value() == string_file_info_key)
		{
			if (has_strings)
			{
				result.add_error(version_info_errc::duplicate_string_file_info);
			}
			else
			{
				load_strings(result, child.get_children());
				has_strings = true;
			}
		}
		else if (key->value() == var_file_info_key)
		{
			if (has_translations)
			{
				result.add_error(version_info_errc::duplicate_var_file_info);
			}
			else
			{
				load_var_file_info(result, child.get_children(), options.allow_virtual_data);
				has_translations = true;
			}
		}
		else
		{
			if (options.on_unknown_block_key)
				options.on_unknown_block_key(child);
		}
	}

	return result;
}

template<typename... Bases>
const std::u16string& version_info_base<Bases...>
	::get_company_name(const std::optional<translation>& lang) const
{
	return get_value_by_key(keys::company_name_key, lang);
}

template<typename... Bases>
const std::u16string& version_info_base<Bases...>
	::get_file_description(const std::optional<translation>& lang) const
{
	return get_value_by_key(keys::file_description_key, lang);
}

template<typename... Bases>
const std::u16string& version_info_base<Bases...>
	::get_file_version(const std::optional<translation>& lang) const
{
	return get_value_by_key(keys::file_version_key, lang);
}

template<typename... Bases>
const std::u16string& version_info_base<Bases...>
	::get_internal_name(const std::optional<translation>& lang) const
{
	return get_value_by_key(keys::internal_name_key, lang);
}

template<typename... Bases>
const std::u16string& version_info_base<Bases...>
	::get_legal_copyright(const std::optional<translation>& lang) const
{
	return get_value_by_key(keys::legal_copyright_key, lang);
}

template<typename... Bases>
const std::u16string& version_info_base<Bases...>
	::get_original_filename(const std::optional<translation>& lang) const
{
	return get_value_by_key(keys::original_filename_key, lang);
}

template<typename... Bases>
const std::u16string& version_info_base<Bases...>
	::get_product_name(const std::optional<translation>& lang) const
{
	return get_value_by_key(keys::product_name_key, lang);
}

template<typename... Bases>
const std::u16string& version_info_base<Bases...>
	::get_product_version(const std::optional<translation>& lang) const
{
	return get_value_by_key(keys::product_version_key, lang);
}

template<typename... Bases>
const std::u16string& version_info_base<Bases...>
	::get_value_by_key(const std::u16string_view key,
	const std::optional<translation>& lang) const
{
	translation_string_map_type::const_iterator lang_it;
	if (lang)
	{
		lang_it = string_values_.find(*lang);
	}
	else
	{
		for (const auto& default_lang : { neutral,
			neutral_process_default, neutral_system_default, english_neutral, english_us })
		{
			lang_it = string_values_.find(default_lang);
			if (lang_it != string_values_.cend())
				break;
		}
	}

	if (lang_it == string_values_.cend())
		throw pe_error(version_info_errc::language_does_not_exist);

	auto string_it = lang_it->second.find(key);
	if (string_it == lang_it->second.cend())
		throw pe_error(version_info_errc::key_does_not_exist);

	return string_it->second;
}

full_version file_version_info::get_file_version() const noexcept
{
	return version_from_components(get_descriptor()->file_version_ms,
		get_descriptor()->file_version_ls);
}

full_version file_version_info::get_product_version() const noexcept
{
	return version_from_components(get_descriptor()->product_version_ms,
		get_descriptor()->product_version_ls);
}

template version_info_base<>;
template version_info_base<error_list>;
template version_info_details get_version_info<>(const version_info_block_base<>& root,
	const version_info_load_options<>& options);
template version_info_details get_version_info<error_list>(
	const version_info_block_base<error_list>& root,
	const version_info_load_options<error_list>& options);

} //namespace pe_bliss::resources
