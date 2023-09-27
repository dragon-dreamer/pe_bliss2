#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <concepts>
#include <span>
#include <utility>
#include <vector>

#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/pkcs7/attribute_map.h"
#include "pe_bliss2/security/pkcs7/signer_info_ref.h"

namespace pe_bliss::security::pkcs7
{

namespace impl
{
[[nodiscard]]
span_range_type decode_octet_string(span_range_type source);
} //namespace impl

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
	std::array<span_range_type, 1u> get_raw_signed_content() const noexcept
		requires(contains_cms_signer_info)
	{
		//content_info_.data.content_info.info.raw includes TAGGED and
		//octet_string_with data, which
		//needs to be skipped. Assumes that content_info_.data.content_info.info
		//is a valid ASN.1 DER.
		const auto& raw = content_info_.data.content_info.info.raw;
		std::uint8_t skip_bytes = 2u; //1 tag byte + 1 length byte for TAGGED
		if (std::to_integer<std::uint8_t>(raw[1]) > 0x80u)
			skip_bytes += std::to_integer<std::uint8_t>(raw[1]) & 0x7fu;

		skip_bytes += 1u; //1 tag byte for octet_string_with
		if (std::to_integer<std::uint8_t>(raw[skip_bytes]) > 0x80u)
			skip_bytes += std::to_integer<std::uint8_t>(raw[skip_bytes]) & 0x7fu;
		skip_bytes += 1u; //1 length byte for octet_string_with

		span_range_type range{
			content_info_.data.content_info.info.raw.begin() + skip_bytes,
			content_info_.data.content_info.info.raw.end()
		};
		return { range };
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

template<typename RangeType>
[[nodiscard]]
bool verify_message_digest_attribute(std::span<const std::byte> calculated_message_digest,
	const attribute_map<RangeType>& signer_authenticated_attributes)
{
	auto message_digest = signer_authenticated_attributes.get_message_digest();
	if (!message_digest)
		return false;
	return std::ranges::equal(
		impl::decode_octet_string(*message_digest),
		calculated_message_digest);
}

} //namespace pe_bliss::security::pkcs7
