#pragma once

#include <cstddef>
#include <vector>

#include "pe_bliss2/security/pkcs7/pkcs7.h"
#include "pe_bliss2/security/pkcs7/attribute_map.h"

namespace pe_bliss::security::pkcs7
{

template<typename RangeType, typename ContentInfo, typename Signer>
[[nodiscard]]
std::vector<std::byte> calculate_message_digest(
	const pkcs7<RangeType, ContentInfo>& signature,
	const Signer& signer)
{
	return signer.calculate_message_digest(signature.get_raw_signed_content());
}

template<typename RangeType1, typename ContentInfo, typename Signer, typename RangeType2>
[[nodiscard]]
bool verify_message_digest(const pkcs7<RangeType1, ContentInfo>& signature,
	const Signer& signer,
	const attribute_map<RangeType2>& authenticated_attributes)
{
	const auto message_digest = calculate_message_digest(signature, signer);
	return verify_message_digest_attribute(message_digest,
		authenticated_attributes);
}

} //namespace pe_bliss::security::pkcs7
