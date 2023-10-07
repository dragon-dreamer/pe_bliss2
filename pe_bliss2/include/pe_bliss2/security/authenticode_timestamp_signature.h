#pragma once

#include <optional>
#include <system_error>
#include <type_traits>
#include <variant>

#include "pe_bliss2/security/pkcs7/pkcs7.h"

#include "simple_asn1/crypto/pkcs7/cms/types.h"
#include "simple_asn1/crypto/pkcs7/types.h"
#include "simple_asn1/crypto/tst/types.h"

namespace pe_bliss::security
{

enum class timestamp_signature_loader_errc
{
	invalid_timestamp_signature_asn1_der = 1
};

std::error_code make_error_code(timestamp_signature_loader_errc) noexcept;

template<typename RangeType>
using authenticode_signature_cms_info_ms_bug_workaround_type
	= pkcs7::pkcs7<RangeType,
		asn1::crypto::pkcs7::cms::ms_bug_workaround::content_info_base<
		asn1::crypto::tst::encap_tst_info<RangeType>, RangeType>>;
template<typename RangeType>
using authenticode_signature_cms_info_type
	= pkcs7::pkcs7<RangeType,
		asn1::crypto::pkcs7::cms::content_info_base<
		asn1::crypto::tst::encap_tst_info<RangeType>, RangeType>>;

template<typename RangeType>
class [[nodiscard]] authenticode_timestamp_signature
{
public:
	using signer_info_type = asn1::crypto::pkcs7::signer_info<RangeType>;
	using cms_info_ms_bug_workaround_type
		= authenticode_signature_cms_info_ms_bug_workaround_type<RangeType>;
	using cms_info_type
		= authenticode_signature_cms_info_type<RangeType>;
	using signature_type = std::variant<signer_info_type,
		cms_info_ms_bug_workaround_type, cms_info_type>;

public:
	[[nodiscard]]
	const signature_type& get_underlying_type() const noexcept
	{
		return signature_;
	}

	[[nodiscard]]
	signature_type& get_underlying_type() noexcept
	{
		return signature_;
	}

private:
	signature_type signature_;
};

template<typename TargetRangeType, typename RangeType>
[[nodiscard]]
std::optional<authenticode_timestamp_signature<TargetRangeType>> load_timestamp_signature(
	const pkcs7::attribute_map<RangeType>& unauthenticated_attrs);

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::timestamp_signature_loader_errc> : true_type {};
} //namespace std
