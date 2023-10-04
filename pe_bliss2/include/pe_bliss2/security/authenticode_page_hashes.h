#pragma once

#include <optional>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/security/authenticode_pkcs7.h"
#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/crypto_algorithms.h"

#include "simple_asn1/crypto/pkcs7/authenticode/types.h"

namespace pe_bliss::security
{

enum class authenticode_page_hashes_errc
{
	invalid_authenticode_page_hashes_asn1,
};

std::error_code make_error_code(authenticode_page_hashes_errc) noexcept;

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

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::authenticode_page_hashes_errc> : true_type {};
} //namespace std
