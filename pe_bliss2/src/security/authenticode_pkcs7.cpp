#include "pe_bliss2/security/authenticode_pkcs7.h"

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/byte_range_types.h"

#include "simple_asn1/crypto/pkcs7/authenticode/spec.h"
#include "simple_asn1/crypto/pkcs7/authenticode/oids.h"
#include "simple_asn1/der_decode.h"

#include "utilities/variant_helpers.h"

namespace pe_bliss::security
{

template<typename RangeType>
span_range_type authenticode_pkcs7<RangeType>::get_image_hash() const noexcept
{
	return this->get_content_info().data.content_info.content.digest.value.digest;
}

template<typename RangeType>
std::optional<program_info<RangeType>> get_program_info(
	const pkcs7::attribute_map<RangeType>& authenticated_attrs)
{
	std::optional<program_info<RangeType>> result;

	const auto info = authenticated_attrs.get_attribute(
		asn1::crypto::pkcs7::authenticode::oid_spc_sp_opus_info);
	if (info)
	{
		//TODO: check return value
		asn1::der::decode<asn1::spec::crypto::pkcs7::authenticode::spc_sp_opus_info>(
			info->begin(), info->end(), result.emplace().get_underlying_info());
	}

	return result;
}

template<typename RangeType>
program_info<RangeType>::string_type program_info<RangeType>
	::get_more_info_url() const noexcept
{
	string_type result;

	if (!info_.more_info.has_value())
		return result;

	std::visit(utilities::overloaded{
		[&result](const std::string& str) { result = str; },
		[&result](const asn1::crypto::pkcs7::authenticode::spc_string_type& str) {
			if (const auto* ascii = std::get_if<std::string>(&str); ascii)
				result = *ascii;
			else
				result = std::get<std::u16string>(str);
		},
		[](const auto&) {},
	}, *info_.more_info);

	return result;
}

template<typename RangeType>
program_info<RangeType>::string_type program_info<RangeType>
	::get_program_name() const noexcept
{
	string_type result;

	if (!info_.program_name.has_value())
		return result;

	if (const auto* ascii = std::get_if<std::string>(&*info_.program_name); ascii)
		result = *ascii;
	else
		result = std::get<std::u16string>(*info_.program_name);

	return result;
}

template class authenticode_pkcs7<span_range_type>;
template class authenticode_pkcs7<vector_range_type>;

template class program_info<span_range_type>;
template class program_info<vector_range_type>;

template std::optional<program_info<span_range_type>> get_program_info(
	const pkcs7::attribute_map<span_range_type>& unauthenticated_attrs);
template std::optional<program_info<vector_range_type>> get_program_info(
	const pkcs7::attribute_map<vector_range_type>& unauthenticated_attrs);

} //namespace pe_bliss::security
