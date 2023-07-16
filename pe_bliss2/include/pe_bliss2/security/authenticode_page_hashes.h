#pragma once

#include <optional>

#include "pe_bliss2/security/authenticode_pkcs7.h"
#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/crypto_algorithms.h"

#include "simple_asn1/crypto/pkcs7/authenticode/types.h"

namespace pe_bliss::security
{

template<typename RangeType>
class [[nodiscard]] authenticode_page_hashes
{
public:
	using packed_page_hashes_type
		= asn1::crypto::pkcs7::authenticode::spc_attribute_page_hashes_set<RangeType>;

public:
	[[nodiscard]]
	std::optional<span_range_type> get_raw_page_hashes() const noexcept;

	[[nodiscard]]
	const packed_page_hashes_type& get_underlying_struct() const noexcept
	{
		return page_hashes_;
	}

	[[nodiscard]]
	packed_page_hashes_type& get_underlying_struct() noexcept
	{
		return page_hashes_;
	}

	[[nodiscard]]
	bool is_valid(digest_algorithm alg) const noexcept;

private:
	packed_page_hashes_type page_hashes_;
};

template<typename TargetRangeType, typename RangeType>
[[nodiscard]]
std::optional<authenticode_page_hashes<TargetRangeType>> get_page_hashes(
	const authenticode_pkcs7<RangeType>& authenticode);

} //namespace pe_bliss::security
