#include "pe_bliss2/security/pkcs7/pkcs7.h"

#include "simple_asn1/der_decode.h"
#include "simple_asn1/spec.h"

namespace pe_bliss::security::pkcs7::impl
{

span_range_type decode_octet_string(span_range_type source)
{
	return asn1::der::decode<span_range_type, asn1::spec::octet_string<>>(
		source.begin(), source.end());
}

} //namespace pe_bliss::security::pkcs7::impl
