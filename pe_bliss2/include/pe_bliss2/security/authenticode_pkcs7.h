#pragma once

#include "simple_asn1/crypto/pkcs7/authenticode/types.h"

#include "pe_bliss2/security/pkcs7/pkcs7.h"

namespace pe_bliss::security
{

template<typename RangeType>
class [[nodiscard]] authenticode_pkcs7 : public pkcs7::pkcs7<RangeType,
	asn1::crypto::pkcs7::authenticode::content_info<RangeType>>
{
public:
	[[nodiscard]]
	span_range_type get_image_hash() const noexcept
	{
		return this->get_content_info()
			.data.content_info.content.digest.value.digest;
	}
};

} //namespace pe_bliss::security
