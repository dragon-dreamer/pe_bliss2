#include "pe_bliss2/security/authenticode_pkcs7.h"

#include <array>

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/byte_range_types.h"

#include "simple_asn1/crypto/pkcs7/authenticode/oids.h"

namespace
{
} //namespace

namespace pe_bliss::security
{

template<typename RangeType>
span_range_type authenticode_pkcs7<RangeType>::get_image_hash() const noexcept
{
	return this->get_content_info().data.content_info.content.digest.value.digest;
}

template class authenticode_pkcs7<span_range_type>;
template class authenticode_pkcs7<vector_range_type>;

} //namespace pe_bliss::security
