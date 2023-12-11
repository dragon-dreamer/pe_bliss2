#pragma once

#include <optional>
#include <utility>

#include "pe_bliss2/security/crypto_algorithms.h"

#include "simple_asn1/crypto/x509/types.h"

namespace pe_bliss::security::x509
{

template<typename RangeType, typename RawCertificateData>
class [[nodiscard]] x509_certificate_base
{
public:
	using range_type = RangeType;

public:
	template<typename Data>
	constexpr x509_certificate_base(Data&& data)
		: data_(std::forward<Data>(data))
	{
	}

	[[nodiscard]]
	const range_type& get_serial_number() const noexcept
	{
		return data_.tbs_cert.serial_number;
	}

	[[nodiscard]]
	const range_type& get_raw_issuer() const noexcept
	{
		return data_.tbs_cert.issuer.raw;
	}

	[[nodiscard]]
	const range_type& get_public_key() const noexcept
	{
		return data_.tbs_cert.pki.subject_publickey.container;
	}

	[[nodiscard]]
	const std::optional<range_type>& get_signature_algorithm_parameters() const noexcept
	{
		return data_.tbs_cert.pki.algorithm.parameters;
	}

	[[nodiscard]]
	const asn1::crypto::algorithm_identifier<range_type>& get_raw_public_key_algorithm() const noexcept
	{
		return data_.tbs_cert.pki.algorithm;
	}

	[[nodiscard]]
	encryption_and_hash_algorithm get_public_key_algorithm() const noexcept
	{
		return get_digest_encryption_algorithm(data_.tbs_cert.pki.algorithm.algorithm.container);
	}

public:
	[[nodiscard]]
	const auto& get_raw_data() const noexcept
	{
		return data_;
	}

	[[nodiscard]]
	auto& get_raw_data() noexcept
	{
		return data_;
	}

private:
	RawCertificateData data_;
};

template<typename RangeType>
using x509_certificate_ref = x509_certificate_base<RangeType,
	const asn1::crypto::x509::certificate<RangeType>&>;
template<typename RangeType>
using x509_certificate = x509_certificate_base<RangeType,
	asn1::crypto::x509::certificate<RangeType>>;

} //namespace pe_bliss::security::x509
