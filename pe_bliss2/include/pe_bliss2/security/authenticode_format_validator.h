#pragma once

#include <optional>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/authenticode_pkcs7.h"

#include "simple_asn1/types.h"

namespace pe_bliss::security
{

template<typename RangeType>
void validate_autenticode_format(
	const authenticode_pkcs7<RangeType>& signature,
	error_list& errors);

template<typename RangeType>
void validate_autenticode_format(
	const pkcs7::attribute_map<RangeType>& authenticated_attributes,
	std::optional<asn1::utc_time>& signing_time, error_list& errors);

} //namespace pe_bliss::security
