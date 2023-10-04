#include "pe_bliss2/security/authenticode_program_info.h"

#include <cstddef>
#include <string>
#include <system_error>

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/asn1_decode_helper.h"

#include "simple_asn1/crypto/pkcs7/authenticode/spec.h"
#include "simple_asn1/crypto/pkcs7/authenticode/oids.h"
#include "simple_asn1/crypto/pkcs7/authenticode/types.h"

#include "utilities/variant_helpers.h"

namespace
{
struct authenticode_program_info_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "authenticode_program_info";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::authenticode_program_info_errc;
		switch (static_cast<pe_bliss::security::authenticode_program_info_errc>(ev))
		{
		case invalid_program_info_asn1_der:
			return "Invalid authenticode program info ASN.1 DER format";
		default:
			return {};
		}
	}
};

const authenticode_program_info_error_category authenticode_program_info_error_category_instance;

} //namespace

namespace pe_bliss::security
{

std::error_code make_error_code(authenticode_program_info_errc e) noexcept
{
	return { static_cast<int>(e), authenticode_program_info_error_category_instance };
}

template<typename RangeType>
std::optional<authenticode_program_info<RangeType>> get_program_info(
	const pkcs7::attribute_map<RangeType>& authenticated_attrs)
{
	std::optional<authenticode_program_info<RangeType>> result;

	const auto info = authenticated_attrs.get_attribute(
		asn1::crypto::pkcs7::authenticode::oid_spc_sp_opus_info);
	if (info)
	{
		decode_asn1_check_tail<
			authenticode_program_info_errc::invalid_program_info_asn1_der,
			asn1::spec::crypto::pkcs7::authenticode::spc_sp_opus_info>(
				*info, result.emplace().get_underlying_info());
	}

	return result;
}

template<typename RangeType>
authenticode_program_info<RangeType>::string_type authenticode_program_info<RangeType>
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
authenticode_program_info<RangeType>::string_type authenticode_program_info<RangeType>
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

template class authenticode_program_info<span_range_type>;
template class authenticode_program_info<vector_range_type>;

template std::optional<authenticode_program_info<span_range_type>> get_program_info(
	const pkcs7::attribute_map<span_range_type>& unauthenticated_attrs);
template std::optional<authenticode_program_info<vector_range_type>> get_program_info(
	const pkcs7::attribute_map<vector_range_type>& unauthenticated_attrs);

} //namespace pe_bliss::security
