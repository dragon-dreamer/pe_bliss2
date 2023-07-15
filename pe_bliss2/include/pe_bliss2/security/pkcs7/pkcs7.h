#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
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
	std::optional<span_range_type> get_message_digest() const;
	[[nodiscard]]
	std::optional<span_range_type> get_content_type() const;

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

template<typename RangeType>
class [[nodiscard]] signer_info_ref
{
public:
	using signer_info_type = asn1::crypto::pkcs7::signer_info<RangeType>;

	constexpr signer_info_ref(const signer_info_type& signer_info_ref) noexcept
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
	digest_encryption_algorithm get_digest_encryption_algorithm() const noexcept
	{
		return pe_bliss::security::get_digest_encryption_algorithm(signer_info_ref_
			.digest_encryption_algorithm.algorithm.container);
	}

	[[nodiscard]]
	attribute_map<RangeType> get_authenticated_attributes() const;

	[[nodiscard]]
	attribute_map<RangeType> get_unauthenticated_attributes() const;

	[[nodiscard]]
	std::vector<std::byte> calculate_message_digest(std::span<const span_range_type> raw_signed_content) const;

	[[nodiscard]]
	std::vector<std::byte> calculate_authenticated_attributes_digest() const;

	[[nodiscard]]
	const RangeType& get_signer_certificate_serial_number() const noexcept
	{
		return signer_info_ref_.issuer_and_sn.serial_number;
	}

	[[nodiscard]]
	const RangeType& get_signer_certificate_raw_issuer() const noexcept
	{
		return signer_info_ref_.issuer_and_sn.issuer.raw;
	}

	[[nodiscard]]
	const RangeType& get_encrypted_digest() const noexcept
	{
		return signer_info_ref_.encrypted_digest;
	}

private:
	const signer_info_type& signer_info_ref_;
};

template<typename RangeType, typename ContentInfo>
class [[nodiscard]] pkcs7
{
public:
	using content_info_type = ContentInfo;

public:
	[[nodiscard]]
	std::size_t get_signer_count() const noexcept
	{
		return content_info_.data.signer_infos.size();
	}

	[[nodiscard]]
	signer_info_ref<RangeType> get_signer(std::size_t index) const
	{
		return content_info_.data.signer_infos.at(index);
	}

	[[nodiscard]]
	std::array<span_range_type, 2u> get_raw_signed_content() const noexcept
	{
		return {
			content_info_.data.content_info.content.type_value.raw,
			content_info_.data.content_info.content.digest.raw
		};
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

template<typename RangeType, typename ContentInfo>
[[nodiscard]]
std::vector<std::byte> calculate_message_digest(const pkcs7<RangeType, ContentInfo>& signature,
	const signer_info_ref<RangeType>& signer)
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
