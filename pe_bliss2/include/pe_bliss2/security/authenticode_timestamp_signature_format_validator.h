#pragma once

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/authenticode_timestamp_signature.h"

#include "simple_asn1/types.h"

namespace pe_bliss::security
{

template<typename RangeType>
void validate_autenticode_timestamp_format(
	const authenticode_signature_cms_info_ms_bug_workaround_type<RangeType>& signature,
	error_list& errors);
template<typename RangeType>
void validate_autenticode_timestamp_format(
	const authenticode_signature_cms_info_type<RangeType>& signature,
	error_list& errors);

template<typename RangeType>
void validate_autenticode_timestamp_format(
	const pkcs7::attribute_map<RangeType>& authenticated_attributes,
	error_list& errors);

} //namespace pe_bliss::security
