#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

namespace pe_bliss::resources
{

enum class guid_errc
{
	invalid_guid = 1
};

std::error_code make_error_code(guid_errc) noexcept;

struct [[nodiscard]] guid
{
	std::uint32_t data1{};
	std::uint16_t data2{};
	std::uint16_t data3{};
	std::array<std::uint8_t, 8u> data4{};

	[[nodiscard]]
	friend constexpr bool operator==(const guid&, const guid&) = default;
};

[[nodiscard]]
guid parse_guid(std::string_view str, bool require_curly_brackets = true);

[[nodiscard]]
std::string to_string(const guid& value, bool add_curly_brackets = true);

} //namespace pe_bliss::resources

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::resources::guid_errc> : true_type {};
} //namespace std
