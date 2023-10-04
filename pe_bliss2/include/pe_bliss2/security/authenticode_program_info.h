#pragma once

#include <optional>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>

#include "simple_asn1/crypto/pkcs7/authenticode/types.h"

#include "pe_bliss2/security/pkcs7/attribute_map.h"

namespace pe_bliss::security
{

enum class authenticode_program_info_errc
{
	invalid_program_info_asn1_der = 1
};

std::error_code make_error_code(authenticode_program_info_errc) noexcept;

template<typename RangeType>
class [[nodiscard]] authenticode_program_info
{
public:
	using spc_sp_opus_info_type
		= asn1::crypto::pkcs7::authenticode::spc_sp_opus_info<RangeType>;

	using string_type = std::variant<
		std::monostate,
		std::reference_wrapper<const std::u16string>,
		std::reference_wrapper<const std::string>>;

public:
	[[nodiscard]]
	spc_sp_opus_info_type& get_underlying_info() noexcept
	{
		return info_;
	}

	[[nodiscard]]
	const spc_sp_opus_info_type& get_underlying_info() const noexcept
	{
		return info_;
	}

public:
	[[nodiscard]]
	string_type get_more_info_url() const noexcept;

	[[nodiscard]]
	string_type get_program_name() const noexcept;

private:
	spc_sp_opus_info_type info_;
};

template<typename RangeType>
[[nodiscard]]
std::optional<authenticode_program_info<RangeType>> get_program_info(
	const pkcs7::attribute_map<RangeType>& authenticated_attrs);

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::authenticode_program_info_errc> : true_type {};
} //namespace std
