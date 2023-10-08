#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <concepts>
#include <span>
#include <utility>
#include <vector>

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/pkcs7/attribute_map.h"
#include "pe_bliss2/security/pkcs7/signer_info.h"

#include "utilities/generic_error.h"

namespace pe_bliss::security::pkcs7
{

template<typename RangeType, typename ContentInfo>
class [[nodiscard]] pkcs7
{
public:
	using content_info_type = ContentInfo;
	static constexpr bool contains_pkcs7_signer_info
		= std::convertible_to<decltype(std::declval<content_info_type>()
			.data.signer_infos)::value_type, signer_info_ref_pkcs7<RangeType>>;
	static constexpr bool contains_cms_signer_info
		= std::convertible_to<decltype(std::declval<content_info_type>()
			.data.signer_infos)::value_type, signer_info_ref_cms<RangeType>>;

public:
	[[nodiscard]]
	std::size_t get_signer_count() const noexcept
	{
		return content_info_.data.signer_infos.size();
	}

	[[nodiscard]]
	signer_info_ref_pkcs7<RangeType> get_signer(std::size_t index) const
		requires(contains_pkcs7_signer_info)
	{
		return content_info_.data.signer_infos.at(index);
	}

	[[nodiscard]]
	signer_info_ref_cms<RangeType> get_signer(std::size_t index) const
		requires(contains_cms_signer_info)
	{
		return content_info_.data.signer_infos.at(index);
	}

	[[nodiscard]]
	std::array<span_range_type, 2u> get_raw_signed_content() const noexcept
		requires(contains_pkcs7_signer_info)
	{
		return {
			content_info_.data.content_info.content.type_value.raw,
			content_info_.data.content_info.content.digest.raw
		};
	}

	[[nodiscard]]
	std::array<span_range_type, 1u> get_raw_signed_content() const
		requires(contains_cms_signer_info)
	{
		//content_info_.data.content_info.info.raw includes TAGGED and
		//octet_string_with data, which
		//needs to be skipped. Assumes that content_info_.data.content_info.info
		//is a valid ASN.1 DER.
		const auto& raw = content_info_.data.content_info.info.raw;
		std::uint8_t skip_bytes = 2u; //1 tag byte + 1 length byte for TAGGED
		if (raw.size() < 2)
			throw pe_error(utilities::generic_errc::buffer_overrun);
		const auto tagged_length_byte = std::to_integer<std::uint8_t>(raw[1]);
		if (tagged_length_byte > 0x80u)
			skip_bytes += tagged_length_byte & 0x7fu;

		skip_bytes += 1u; //1 tag byte for octet_string_with
		if (raw.size() < skip_bytes + 1)
			throw pe_error(utilities::generic_errc::buffer_overrun);
		const auto octet_string_length_byte = std::to_integer<std::uint8_t>(
			raw[skip_bytes]);
		if (octet_string_length_byte > 0x80u)
			skip_bytes += octet_string_length_byte & 0x7fu;
		skip_bytes += 1u; //1 length byte for octet_string_with

		if (raw.size() < skip_bytes + 1)
			throw pe_error(utilities::generic_errc::buffer_overrun);
		return { span_range_type{ raw.begin() + skip_bytes, raw.end() } };
	}

public:
	[[nodiscard]]
	const content_info_type& get_content_info() const noexcept
	{
		return content_info_;
	}

	[[nodiscard]]
	content_info_type& get_content_info() noexcept
	{
		return content_info_;
	}

private:
	content_info_type content_info_;
};

template<typename RangeType, typename ContentInfo, typename Signer>
[[nodiscard]]
std::vector<std::byte> calculate_message_digest(
	const pkcs7<RangeType, ContentInfo>& signature,
	const Signer& signer)
{
	return signer.calculate_message_digest(signature.get_raw_signed_content());
}

} //namespace pe_bliss::security::pkcs7
