#include "pe_bliss2/security/authenticode_pkcs7.h"

#include <array>

#include "pe_bliss2/pe_error.h"

namespace
{
constexpr std::array spc_sp_opus_info_objid {
	1u, 3u, 6u, 1u, 4u, 1u, 311u, 2u, 1u, 12u
};
} //namespace

namespace pe_bliss::security
{

template<typename RangeType>
pkcs7::span_range_type authenticode_pkcs7<RangeType>::get_image_hash() const noexcept
{
	return this->get_content_info().data.content_info.value.content.digest.digest;
}

template class authenticode_pkcs7<pkcs7::span_range_type>;
template class authenticode_pkcs7<pkcs7::vector_range_type>;

} //namespace pe_bliss::security
