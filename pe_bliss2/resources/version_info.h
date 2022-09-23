#pragma once

#include <compare>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <sstream>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>

#include "pe_bliss2/detail/resources/version_info.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/resources/version_info_block.h"

namespace pe_bliss::resources
{

struct [[nodiscard]] translation final
{
	std::uint16_t lcid{};
	std::uint16_t cpid{};

	[[nodiscard]]
	friend bool operator==(const translation& l, const translation& r) = default;
	[[nodiscard]]
	friend auto operator<=>(const translation& l, const translation& r) = default;
};

enum class version_info_errc
{
	language_does_not_exist = 1,
	key_does_not_exist,
	incorrect_root_block_key,
	absent_file_version_info,
	file_version_info_read_error,
	unknown_version_info_key,
	invalid_var_file_info_children,
	invalid_translations,
	absent_translations,
	duplicate_translations,
	invalid_string_translations,
	duplicate_string_translations,
	invalid_strings,
	duplicate_strings
};

std::error_code make_error_code(version_info_errc) noexcept;

} //namespace pe_bliss::resources

namespace std
{
template<>
struct hash<pe_bliss::resources::translation> final
{
	[[nodiscard]]
	std::size_t operator()(
		const pe_bliss::resources::translation& value) const noexcept
	{
		return std::hash<std::uint32_t>()((value.lcid << 16u) | value.cpid);
	}
};

template<>
struct is_error_code_enum<pe_bliss::resources::version_info_errc> : true_type {};
} //namespace std

namespace pe_bliss::resources
{

enum class file_os
{
	unknown = detail::resources::vos_unknown,
	dos = detail::resources::vos_dos,
	nt = detail::resources::vos_nt,
	wince = detail::resources::vos_wince,
	windows16 = detail::resources::vos__windows16,
	windows32 = detail::resources::vos__windows32,
	dos_windows16 = detail::resources::vos_dos_windows16,
	dos_windows32 = detail::resources::vos_dos_windows32,
	os216_pm16 = detail::resources::vos_os216_pm16,
	os232_pm32 = detail::resources::vos_os232_pm32,
	nt_windows32 = detail::resources::vos_nt_windows32
};

enum class file_type
{
	unknown = detail::resources::vft_unknown,
	app = detail::resources::vft_app,
	dll = detail::resources::vft_dll,
	drv = detail::resources::vft_drv,
	font = detail::resources::vft_font,
	vxd = detail::resources::vft_vxd,
	static_lib = detail::resources::vft_static_lib
};

enum class driver_file_subtype
{
	unknown = detail::resources::vft2_unknown,
	printer = detail::resources::vft2_drv_printer,
	keyboard = detail::resources::vft2_drv_keyboard,
	language = detail::resources::vft2_drv_language,
	display = detail::resources::vft2_drv_display,
	mouse = detail::resources::vft2_drv_mouse,
	network = detail::resources::vft2_drv_network,
	system = detail::resources::vft2_drv_system,
	installable = detail::resources::vft2_drv_installable,
	sound = detail::resources::vft2_drv_sound,
	comm = detail::resources::vft2_drv_comm,
	inputmethod = detail::resources::vft2_drv_inputmethod,
	versioned_printer = detail::resources::vft2_drv_versioned_printer
};

enum class font_file_subtype
{
	font_raster = detail::resources::vft2_font_raster,
	font_vector = detail::resources::vft2_font_vector,
	font_truetype = detail::resources::vft2_font_truetype
};

struct file_flags final
{
	enum value
	{
		debug = detail::resources::vs_ff_debug,
		prerelease = detail::resources::vs_ff_prerelease,
		patched = detail::resources::vs_ff_patched,
		privatebuild = detail::resources::vs_ff_privatebuild,
		infoinferred = detail::resources::vs_ff_infoinferred,
		specialbuild = detail::resources::vs_ff_specialbuild
	};
};

class [[nodiscard]] file_version_info
{
public:
	using descriptor_type = packed_struct<detail::resources::vs_fixedfileinfo>;
	using file_subtype_type = std::variant<
		std::monostate, driver_file_subtype, font_file_subtype>;

public:
	[[nodiscard]]
	descriptor_type& get_descriptor() noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	const descriptor_type& get_descriptor() const noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	file_os get_file_os() const noexcept
	{
		return static_cast<file_os>(descriptor_->file_os);
	}

	[[nodiscard]]
	file_type get_file_type() const noexcept
	{
		return static_cast<file_type>(descriptor_->file_type);
	}

	[[nodiscard]]
	file_flags::value get_file_flags() const noexcept
	{
		return static_cast<file_flags::value>(descriptor_->file_flags);
	}

	[[nodiscard]]
	file_subtype_type get_file_subtype() const noexcept
	{
		if (get_file_type() == file_type::drv)
			return static_cast<driver_file_subtype>(descriptor_->file_subtype);
		if (get_file_type() == file_type::font)
			return static_cast<font_file_subtype>(descriptor_->file_subtype);

		return {};
	}

	template<typename CharType = char>
	[[nodiscard]]
	std::basic_string<CharType> get_file_version_string() const
	{
		return get_version_string<CharType>(descriptor_->file_version_ms,
			descriptor_->file_version_ls);
	}

	template<typename CharType = char>
	[[nodiscard]]
	std::basic_string<CharType> get_product_version_string() const
	{
		return get_version_string<CharType>(descriptor_->product_version_ms,
			descriptor_->product_version_ls);
	}

private:
	template<typename CharType>
	[[nodiscard]]
	static std::basic_string<CharType> get_version_string(
		std::uint32_t ms, std::uint32_t ls)
	{
		std::basic_stringstream<CharType> ss;
		ss << (ms >> 16u) << ss.widen(L'.')
			<< (ms & 0xffffu) << ss.widen(L'.')
			<< (ls >> 16u) << ss.widen(L'.')
			<< (ls & 0xffffu);
		return ss.str();
	}

private:
	descriptor_type descriptor_;
};

namespace impl
{
struct transparent_string_hash
{
	template<typename T>
	auto operator()(const T& str) const noexcept
	{
		return std::hash<T>{}(str);
	}

	using is_transparent = void;
};
} //namespace impl

using translation_set = std::unordered_set<translation>;
using string_value_map_type = std::unordered_map<std::u16string, std::u16string,
	impl::transparent_string_hash, std::equal_to<>>;
using translation_string_map_type = std::unordered_map<translation, string_value_map_type>;

namespace keys
{
constexpr std::u16string_view company_name_key(u"CompanyName");
constexpr std::u16string_view file_description_key(u"FileDescription");
constexpr std::u16string_view file_version_key(u"FileVersion");
constexpr std::u16string_view internal_name_key(u"InternalName");
constexpr std::u16string_view legal_copyright_key(u"LegalCopyright");
constexpr std::u16string_view original_filename_key(u"OriginalFilename");
constexpr std::u16string_view product_name_key(u"ProductName");
constexpr std::u16string_view product_version_key(u"ProductVersion");
} //namespace keys

template<typename... Bases>
class [[nodiscard]] version_info_base : public Bases...
{
public:
	[[nodiscard]]
	file_version_info& get_file_version_info() noexcept
	{
		return file_version_info_;
	}

	[[nodiscard]]
	const file_version_info& get_file_version_info() const noexcept
	{
		return file_version_info_;
	}

	[[nodiscard]]
	translation_string_map_type& get_strings() & noexcept
	{
		return string_values_;
	}

	[[nodiscard]]
	const translation_string_map_type& get_strings() const& noexcept
	{
		return string_values_;
	}

	[[nodiscard]]
	translation_string_map_type get_strings() && noexcept
	{
		return std::move(string_values_);
	}

	[[nodiscard]]
	translation_set& get_translations() & noexcept
	{
		return translations_;
	}

	[[nodiscard]]
	const translation_set& get_translations() const& noexcept
	{
		return translations_;
	}

	[[nodiscard]]
	translation_set get_translations() && noexcept
	{
		return std::move(translations_);
	}

public:
	[[nodiscard]]
	const std::u16string& get_company_name(const std::optional<translation>& lang = {}) const;
	[[nodiscard]]
	const std::u16string& get_file_description(const std::optional<translation>& lang = {}) const;
	[[nodiscard]]
	const std::u16string& get_file_version(const std::optional<translation>& lang = {}) const;
	[[nodiscard]]
	const std::u16string& get_internal_name(const std::optional<translation>& lang = {}) const;
	[[nodiscard]]
	const std::u16string& get_legal_copyright(const std::optional<translation>& lang = {}) const;
	[[nodiscard]]
	const std::u16string& get_original_filename(const std::optional<translation>& lang = {}) const;
	[[nodiscard]]
	const std::u16string& get_product_name(const std::optional<translation>& lang = {}) const;
	[[nodiscard]]
	const std::u16string& get_product_version(const std::optional<translation>& lang = {}) const;
	[[nodiscard]]
	const std::u16string& get_value_by_key(const std::u16string_view key,
		const std::optional<translation>& lang = {}) const;

private:
	file_version_info file_version_info_;
	translation_string_map_type string_values_;
	translation_set translations_;
};

using version_info = version_info_base<>;
using version_info_details = version_info_base<error_list>;

template<typename... Bases>
[[nodiscard]]
version_info_details get_version_info(const version_info_block_base<Bases...>& root,
	bool allow_virtual_memory = false);

} //namespace pe_bliss::resources
