#include "pe_bliss2/security/authenticode_timestamp_signature.h"

#include <string>

#include "pe_bliss2/security/asn1_decode_helper.h"
#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/pe_error.h"

#include "simple_asn1/crypto/pkcs7/authenticode/oids.h"
#include "simple_asn1/crypto/pkcs7/cms/spec.h"
#include "simple_asn1/crypto/pkcs7/spec.h"
#include "simple_asn1/crypto/pkcs9/oids.h"
#include "simple_asn1/crypto/tst/spec.h"
#include "simple_asn1/der_decode.h"

namespace
{

struct timestamp_signature_loader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "timestamp_signature_loader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::timestamp_signature_loader_errc;
		switch (static_cast<pe_bliss::security::timestamp_signature_loader_errc>(ev))
		{
		case invalid_timestamp_signature_asn1_der:
			return "Unable to read the PKCS7/CMS timestamp signature ASN.1 DER";
		default:
			return {};
		}
	}
};

const timestamp_signature_loader_error_category timestamp_signature_loader_error_category_instance;

} //namespace

namespace pe_bliss::security
{

std::error_code make_error_code(timestamp_signature_loader_errc e) noexcept
{
	return { static_cast<int>(e), timestamp_signature_loader_error_category_instance };
}

template<typename TargetRangeType, typename RangeType>
std::optional<authenticode_timestamp_signature<TargetRangeType>> load_timestamp_signature(
	const pkcs7::attribute_map<RangeType>& unauthenticated_attrs)
{
	std::optional<authenticode_timestamp_signature<TargetRangeType>> result;

	auto raw_signature = unauthenticated_attrs.get_attribute(
		asn1::crypto::pkcs9::oid_timestamp_token);
	if (!raw_signature)
	{
		raw_signature = unauthenticated_attrs.get_attribute(
			asn1::crypto::pkcs7::authenticode::oid_spc_time_stamp_token);
	}

	if (raw_signature)
	{
		using cms_info_spec_ms_bug_workaround_type = asn1::spec::crypto::pkcs7::cms
			::ms_bug_workaround::content_info_base<asn1::spec::crypto::tst::encap_tst_info>;
		using cms_info_spec_type = asn1::spec::crypto::pkcs7::cms
			::content_info_base<asn1::spec::crypto::tst::encap_tst_info>;

		auto& underlying_variant = result.emplace().get_underlying_type();

		try
		{
			decode_asn1_check_tail<
				timestamp_signature_loader_errc::invalid_timestamp_signature_asn1_der,
				cms_info_spec_ms_bug_workaround_type>(
					*raw_signature, underlying_variant
					.template emplace<typename authenticode_timestamp_signature<TargetRangeType>
					::cms_info_ms_bug_workaround_type>().get_content_info());
		}
		catch (const pe_error&)
		{
			decode_asn1_check_tail<
				timestamp_signature_loader_errc::invalid_timestamp_signature_asn1_der,
				cms_info_spec_type>(
					*raw_signature, underlying_variant
					.template emplace<typename authenticode_timestamp_signature<TargetRangeType>
					::cms_info_type>().get_content_info());
		}

		return result;
	}

	raw_signature = unauthenticated_attrs.get_attribute(
			asn1::crypto::pkcs9::oid_counter_signature);

	if (raw_signature)
	{
		auto& underlying_variant = result.emplace().get_underlying_type();

		decode_asn1_check_tail<
			timestamp_signature_loader_errc::invalid_timestamp_signature_asn1_der,
			asn1::spec::crypto::pkcs7::signer_info>(
				*raw_signature, underlying_variant
				.template emplace<typename authenticode_timestamp_signature<TargetRangeType>
				::signer_info_type>().get_underlying());
	}

	return result;
}

template class authenticode_timestamp_signature<span_range_type>;
template class authenticode_timestamp_signature<vector_range_type>;
template std::optional<authenticode_timestamp_signature<span_range_type>> load_timestamp_signature<
	span_range_type>(
		const pkcs7::attribute_map<span_range_type>& unauthenticated_attrs);
template std::optional<authenticode_timestamp_signature<vector_range_type>> load_timestamp_signature<
	vector_range_type>(
		const pkcs7::attribute_map<vector_range_type>& unauthenticated_attrs);
template std::optional<authenticode_timestamp_signature<span_range_type>> load_timestamp_signature<
	span_range_type>(
		const pkcs7::attribute_map<vector_range_type>& unauthenticated_attrs);
template std::optional<authenticode_timestamp_signature<vector_range_type>> load_timestamp_signature<
	vector_range_type>(
		const pkcs7::attribute_map<span_range_type>& unauthenticated_attrs);

} //namespace pe_bliss::security
