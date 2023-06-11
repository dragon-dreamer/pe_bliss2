#include "pe_bliss2/resources/guid.h"

#include <cstddef>
#include <charconv>
#ifdef __cpp_lib_format
#	include <format>
#else
#	include <cstdio>
#endif //__cpp_lib_format
#include <string>
#include <system_error>

#include "pe_bliss2/pe_error.h"

namespace
{
struct guid_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "guid";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::resources::guid_errc;
		switch (static_cast<pe_bliss::resources::guid_errc>(ev))
		{
		case invalid_guid:
			return "Invalid GUID";
		default:
			return {};
		}
	}
};

const guid_category guid_category_instance;
} //namespace

namespace pe_bliss::resources
{

std::error_code make_error_code(guid_errc e) noexcept
{
	return { static_cast<int>(e), guid_category_instance };
}

namespace
{
template<typename T>
const char* parse_part(const char* ptr, T& value, bool require_dash = true)
{
	auto res = std::from_chars(ptr, ptr + sizeof(T) * 2u, value, 16u);
	if (res.ec != std::errc{} || res.ptr != ptr + sizeof(T) * 2u)
		throw pe_error(guid_errc::invalid_guid);

	ptr += sizeof(T) * 2u;
	if (require_dash && *ptr++ != '-')
		throw pe_error(guid_errc::invalid_guid);

	return ptr;
}
}

guid parse_guid(std::string_view str, bool require_curly_brackets)
{
	static constexpr std::size_t expected_length = 36u;
	if (str.size() != expected_length + require_curly_brackets * 2u)
		throw pe_error(guid_errc::invalid_guid);

	if (require_curly_brackets)
	{
		if (str.front() != '{' || str.back() != '}')
			throw pe_error(guid_errc::invalid_guid);
		
		str.remove_prefix(1u);
	}

	guid result{};
	const char* ptr = parse_part(str.data(), result.data1);
	ptr = parse_part(ptr, result.data2);
	ptr = parse_part(ptr, result.data3);
	ptr = parse_part(ptr, result.data4[0], false);
	ptr = parse_part(ptr, result.data4[1], true);
	for (std::size_t i = 2u; i != 8u; ++i)
		ptr = parse_part(ptr, result.data4[i], false);

	return result;
}

namespace
{
struct guid_formatter
{
	static constexpr std::string_view get()
	{
		return "{:08X}-{:04X}-{:04X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}";
	}

	static constexpr const char* get_legacy()
	{
		return "%08X-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX";
	}
};

struct guid_bracket_formatter
{
	static constexpr std::string_view get()
	{
		return "{{{:08X}-{:04X}-{:04X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}}}";
	}

	static constexpr const char* get_legacy()
	{
		return "{%08X-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}";
	}
};

template<typename Formatter>
std::string to_string(const guid& value)
{
#ifdef __cpp_lib_format
	return std::format(
		Formatter::get(),
		value.data1, value.data2, value.data3,
		value.data4[0], value.data4[1],
		value.data4[2], value.data4[3],
		value.data4[4], value.data4[5],
		value.data4[6], value.data4[7]);
#else //__cpp_lib_format
	char buf[64]{};
	std::sprintf(
		buf,
		Formatter::get_legacy(),
		value.data1, value.data2, value.data3,
		value.data4[0], value.data4[1],
		value.data4[2], value.data4[3],
		value.data4[4], value.data4[5],
		value.data4[6], value.data4[7]);
	return buf;
#endif //__cpp_lib_format
}
} //namespace

std::string to_string(const guid& value, bool add_curly_brackets)
{
	if (add_curly_brackets)
		return to_string<guid_bracket_formatter>(value);

	return to_string<guid_formatter>(value);
}

} //namespace pe_bliss::resources
