#pragma once

#include <algorithm>
#include <cstddef>
#include <unordered_map>
#include <utility>

#include "simple_asn1/crypto/crypto_common_types.h"

#include "pe_bliss2/security/byte_range_types.h"

#include "utilities/hash.h"
#include "utilities/range_helpers.h"

namespace pe_bliss::security::x509
{

template<typename Certificate>
class [[nodiscard]] x509_certificate_store
{
public:
	using range_type = typename Certificate::range_type;

public:
	void reserve(std::size_t count)
	{
		serial_number_to_certificate_.reserve(count);
	}

	bool add_certificate(Certificate&& certificate)
	{
		return serial_number_to_certificate_.emplace(
			sn_with_issuer{ certificate.get_serial_number(), certificate.get_raw_issuer() },
			std::move(certificate)).second;
	}

	[[nodiscard]]
	const Certificate* find_certificate(span_range_type serial_number,
		span_range_type raw_issuer) const
	{
		auto it = serial_number_to_certificate_.find(
			sn_with_issuer{ serial_number, raw_issuer });
		if (it == serial_number_to_certificate_.cend())
			return nullptr;

		return &it->second;
	}

	[[nodiscard]]
	std::size_t size() const noexcept
	{
		return serial_number_to_certificate_.size();
	}

	[[nodiscard]]
	bool empty() const noexcept
	{
		return serial_number_to_certificate_.empty();
	}

private:
	struct sn_with_issuer
	{
		span_range_type serial_number;
		span_range_type raw_issuer;

		friend bool operator==(const sn_with_issuer& l, const sn_with_issuer& r) noexcept
		{
			return std::ranges::equal(l.serial_number, r.serial_number)
				&& std::ranges::equal(l.raw_issuer, r.raw_issuer);
		}
	};

	struct sn_with_issuer_hash
	{
		std::size_t operator()(const sn_with_issuer& data) const
		{
			std::size_t hash = utilities::range_hash{}(data.serial_number);
			utilities::hash_combine(hash, utilities::range_hash{}(data.raw_issuer));
			return hash;
		}
	};

private:
	std::unordered_map<sn_with_issuer, Certificate,
		sn_with_issuer_hash> serial_number_to_certificate_;
};

} //namespace pe_bliss::security::x509
