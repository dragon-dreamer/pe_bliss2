#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <concepts>
#include <optional>
#include <span>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/crypto_algorithms.h"

#include "simple_asn1/crypto/pkcs7/types.h"
#include "simple_asn1/crypto/pkcs7/cms/types.h"

#include "utilities/range_helpers.h"

namespace pe_bliss::security::pkcs7
{

enum class pkcs7_errc
{
	duplicate_attribute_oid,
	absent_attribute_value,
	multiple_attribute_values,
	absent_authenticated_attributes
};

std::error_code make_error_code(pkcs7_errc) noexcept;

namespace impl
{
[[nodiscard]]
span_range_type decode_octet_string(span_range_type source);
} //namespace impl

template<typename RangeType>
using attribute_map_type = std::unordered_map<
	std::span<const std::uint32_t>, std::reference_wrapper<const std::vector<RangeType>>,
	utilities::range_hash, utilities::range_equal
>;

template<typename RangeType>
class attribute_map
{
public:
	[[nodiscard]]
	std::optional<span_range_type> get_attribute(
		std::span<const std::uint32_t> oid) const;

	[[nodiscard]]
	std::vector<span_range_type> get_attributes(
		std::span<const std::uint32_t> oid) const;

	[[nodiscard]]
	std::optional<span_range_type> get_message_digest() const;
	[[nodiscard]]
	std::optional<span_range_type> get_content_type() const;
	[[nodiscard]]
	std::optional<span_range_type> get_signing_time() const;

public:
	[[nodiscard]]
	attribute_map_type<RangeType>& get_map() & noexcept
	{
		return map_;
	}

	[[nodiscard]]
	const attribute_map_type<RangeType>& get_map() const& noexcept
	{
		return map_;
	}

	[[nodiscard]]
	attribute_map_type<RangeType> get_map() && noexcept
	{
		return std::move(map_);
	}

private:
	attribute_map_type<RangeType> map_;
};

template<typename RangeType, typename SignerInfoType>
class [[nodiscard]] signer_info_ref_base
{
public:
	using signer_info_type = SignerInfoType;

	constexpr signer_info_ref_base(const signer_info_type& signer_info_ref) noexcept
		: signer_info_ref_(signer_info_ref)
	{
	}

public:
	[[nodiscard]]
	digest_algorithm get_digest_algorithm() const noexcept
	{
		return pe_bliss::security::get_digest_algorithm(signer_info_ref_
			.digest_algorithm.algorithm.container);
	}

	[[nodiscard]]
	encryption_and_hash_algorithm get_digest_encryption_algorithm() const noexcept
	{
		return pe_bliss::security::get_digest_encryption_algorithm(signer_info_ref_
			.digest_encryption_algorithm.algorithm.container);
	}

	[[nodiscard]]
	attribute_map<RangeType> get_authenticated_attributes() const;

	[[nodiscard]]
	attribute_map<RangeType> get_unauthenticated_attributes() const;

	[[nodiscard]]
	std::vector<std::byte> calculate_message_digest(
		std::span<const span_range_type> raw_signed_content) const;

	[[nodiscard]]
	std::vector<std::byte> calculate_authenticated_attributes_digest() const;

	[[nodiscard]]
	const RangeType& get_encrypted_digest() const noexcept
	{
		return signer_info_ref_.encrypted_digest;
	}

public:
	[[nodiscard]]
	const signer_info_type& get_underlying() const noexcept
	{
		return signer_info_ref_;
	}

private:
	const signer_info_type& signer_info_ref_;
};

template<typename RangeType>
struct cert_issuer_and_serial_number
{
	const RangeType* issuer;
	const RangeType* serial_number;
};

template<typename RangeType>
class [[nodiscard]] signer_info_ref_pkcs7 : public signer_info_ref_base<
	RangeType, asn1::crypto::pkcs7::signer_info<RangeType>>
{
public:
	using signer_info_ref_base<RangeType,
		asn1::crypto::pkcs7::signer_info<RangeType>>::signer_info_ref_base;

public:
	[[nodiscard]]
	cert_issuer_and_serial_number<RangeType>
		get_signer_certificate_issuer_and_serial_number() const noexcept
	{
		return {
			.issuer = &this->get_underlying().issuer_and_sn.issuer.raw,
			.serial_number = &this->get_underlying().issuer_and_sn.serial_number
		};
	}
};

template<typename RangeType>
class [[nodiscard]] signer_info_ref_cms : public signer_info_ref_base<
	RangeType, asn1::crypto::pkcs7::cms::signer_info<RangeType>>
{
public:
	using signer_info_ref_base<RangeType,
		asn1::crypto::pkcs7::cms::signer_info<RangeType>>::signer_info_ref_base;

public:
	[[nodiscard]]
	cert_issuer_and_serial_number<RangeType>
		get_signer_certificate_issuer_and_serial_number() const noexcept
	{
		const auto* issuer_and_sn = std::get_if<
			asn1::crypto::pkcs7::issuer_and_serial_number<RangeType>>(
				&this->get_underlying().sid);
		if (!issuer_and_sn)
			return {};

		return {
			.issuer = &issuer_and_sn->issuer.raw,
			.serial_number = &issuer_and_sn->serial_number
		};
	}
};

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

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::pkcs7::pkcs7_errc> : true_type {};
} //namespace std
