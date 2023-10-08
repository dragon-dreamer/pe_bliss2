#pragma once

#include <cstddef>
#include <span>
#include <system_error>
#include <type_traits>
#include <variant>
#include <vector>

#include "simple_asn1/crypto/pkcs7/types.h"
#include "simple_asn1/crypto/pkcs7/cms/types.h"

#include "pe_bliss2/security/crypto_algorithms.h"
#include "pe_bliss2/security/pkcs7/attribute_map.h"

namespace pe_bliss::security::pkcs7
{

enum class signer_info_errc
{
	duplicate_attribute_oid = 1,
	absent_authenticated_attributes
};

std::error_code make_error_code(signer_info_errc) noexcept;

template<typename RangeType, typename SignerInfoType, typename UnderlyingType>
class [[nodiscard]] signer_info_base
{
public:
	using signer_info_type = SignerInfoType;

	constexpr signer_info_base(UnderlyingType signer_info_ref) noexcept
		: signer_info_ref_(signer_info_ref)
	{
	}

public:
	[[nodiscard]]
	int get_version() const noexcept
	{
		return signer_info_ref_.version;
	}

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
	UnderlyingType signer_info_ref_;
};

template<typename RangeType, typename SignerInfoType>
class [[nodiscard]] signer_info_ref_base
	: public signer_info_base<RangeType, SignerInfoType, const SignerInfoType&>
{
public:
	using signer_info_base<RangeType, SignerInfoType, const SignerInfoType&>
		::signer_info_base;
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

} //namespace pe_bliss::security::pkcs7

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::pkcs7::signer_info_errc> : true_type {};
} //namespace std
