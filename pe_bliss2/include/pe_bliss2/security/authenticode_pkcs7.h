#pragma once

#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "simple_asn1/crypto/pkcs7/authenticode/types.h"

#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/pkcs7/pkcs7.h"

namespace pe_bliss::security
{

template<typename RangeType>
class [[nodiscard]] authenticode_pkcs7 : public pkcs7::pkcs7<RangeType,
	asn1::crypto::pkcs7::authenticode::content_info<RangeType>>
{
public:
	[[nodiscard]]
	span_range_type get_image_hash() const noexcept;
};

template<typename RangeType>
class [[nodiscard]] program_info
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
std::optional<program_info<RangeType>> get_program_info(
	const pkcs7::attribute_map<RangeType>& authenticated_attrs);

} //namespace pe_bliss::security
