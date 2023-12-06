#pragma once

#include "pe_bliss2/security/authenticode_check_status.h"
#include "pe_bliss2/security/authenticode_pkcs7.h"
#include "pe_bliss2/security/image_hash.h"
#include "pe_bliss2/security/pkcs7/pkcs7.h"
#include "pe_bliss2/security/pkcs7/pkcs7_signature.h"

#include "simple_asn1/types.h"

namespace pe_bliss::image { class image; }

namespace pe_bliss::security
{

struct [[nodiscard]] authenticode_verification_options final
{
	page_hash_options page_hash_opts;
};

template<typename RangeType>
[[nodiscard]]
authenticode_check_status<RangeType> verify_authenticode_full(
	const authenticode_pkcs7<RangeType>& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts = {});

template<typename RangeType>
void verify_authenticode(const authenticode_pkcs7<RangeType>& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base<RangeType>& result);

// Requires cert_store to be set inside authenticode_check_status_base
template<typename RangeType>
void verify_valid_format_authenticode(
	const authenticode_pkcs7<RangeType>& authenticode,
	const pkcs7::signer_info_ref_pkcs7<RangeType>& signer,
	const pkcs7::attribute_map<RangeType>& authenticated_attributes,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base<RangeType>& result);

} //namespace pe_bliss::security
