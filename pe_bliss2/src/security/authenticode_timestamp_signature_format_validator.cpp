#include "pe_bliss2/security/authenticode_timestamp_signature_format_validator.h"

#include <algorithm>
#include <optional>
#include <variant>

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/authenticode_format_validator_errc.h"
#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/pkcs7/pkcs7_format_validator.h"
#include "pe_bliss2/security/pkcs7/signer_info.h"

#include "simple_asn1/crypto/crypto_common_types.h"
#include "simple_asn1/crypto/pkcs7/types.h"
#include "simple_asn1/crypto/pkcs9/oids.h"

#include "utilities/variant_helpers.h"

namespace
{

struct authenticode_timestamp_signature_format_validator_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "authenticode_timestamp_signature_format_validator";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::authenticode_timestamp_signature_format_validator_errc;
		switch (static_cast<pe_bliss::security::authenticode_timestamp_signature_format_validator_errc>(ev))
		{
		case invalid_tst_info_version:
			return "Invalid TSTInfo version number (must be 1)";
		case invalid_tst_info_accuracy_value:
			return "Invalid TSTInfo Accuracy value";
		default:
			return {};
		}
	}
};

const authenticode_timestamp_signature_format_validator_error_category
	authenticode_timestamp_signature_format_validator_category_instance;

} //namespace

namespace pe_bliss::security
{

namespace
{

void validate_accuracy(const std::optional<int>& value,
	error_list& errors)
{
	if (!value.has_value())
		return;

	if (*value < 1 || *value > 999)
	{
		errors.add_error(
			authenticode_timestamp_signature_format_validator_errc::invalid_tst_info_accuracy_value);
	}
}

template<typename T>
void validate_autenticode_timestamp_format_impl(
	const T& signature, error_list& errors)
{
	static constexpr std::int32_t cms_info_version = 3u;
	pkcs7::validate(signature, errors, cms_info_version);

	const auto& content_info = signature.get_content_info();
	if (content_info.data.digest_algorithms.size() != 1u
		|| content_info.data.signer_infos.size() != 1u)
	{
		errors.add_error(pkcs7::pkcs7_format_validator_errc::invalid_signer_count);
		return;
	}

	const auto& signed_data_content_info = content_info.data.content_info;
	if (!std::ranges::equal(signed_data_content_info.content_type.container,
		asn1::crypto::pkcs9::oid_tst_info))
	{
		errors.add_error(authenticode_format_validator_errc::invalid_content_info_oid);
	}

	const auto& tst_info = signed_data_content_info.info.value;
	static constexpr std::int32_t tst_info_version = 1u;
	if (tst_info.version != tst_info_version)
	{
		errors.add_error(
			authenticode_timestamp_signature_format_validator_errc::invalid_tst_info_version);
	}

	if (tst_info.accuracy_val.has_value())
	{
		validate_accuracy(tst_info.accuracy_val->millis, errors);
		validate_accuracy(tst_info.accuracy_val->micros, errors);
	}
}

} //namespace

std::error_code make_error_code(authenticode_timestamp_signature_format_validator_errc e) noexcept
{
	return { static_cast<int>(e),
		authenticode_timestamp_signature_format_validator_category_instance };
}

template<typename RangeType>
void validate_autenticode_timestamp_format(
	const authenticode_signature_cms_info_type<RangeType>& signature,
	error_list& errors)
{
	validate_autenticode_timestamp_format_impl(signature, errors);
}

template<typename RangeType>
void validate_autenticode_timestamp_format(
	const authenticode_signature_cms_info_ms_bug_workaround_type<RangeType>& signature,
	error_list& errors)
{
	validate_autenticode_timestamp_format_impl(signature, errors);
}

template<typename RangeType>
void validate_autenticode_timestamp_format(
	const pkcs7::attribute_map<RangeType>& authenticated_attributes,
	error_list& errors)
{
	std::optional<asn1::crypto::object_identifier_type> content_type;
	std::optional<asn1::utc_time> signing_time;
	pkcs7::validate_authenticated_attributes(authenticated_attributes,
		signing_time, content_type, errors);

	if (content_type && !std::ranges::equal(content_type->container,
		asn1::crypto::pkcs9::oid_tst_info))
	{
		errors.add_error(pkcs7::pkcs7_format_validator_errc::invalid_content_type);
	}
}

template<typename RangeType>
void validate_autenticode_timestamp_format(
	const authenticode_timestamp_signature<RangeType>& signature,
	error_list& errors)
{
	std::visit(utilities::overloaded{
		[&errors](const pkcs7::signer_info_pkcs7<RangeType>& underlying) {
			return validate_autenticode_timestamp_format(
				underlying.get_authenticated_attributes(), errors);
		},
		[&errors](const auto& underlying) {
			return validate_autenticode_timestamp_format(underlying, errors);
		}
	}, signature.get_underlying_type());
}

template void validate_autenticode_timestamp_format<span_range_type>(
	const authenticode_signature_cms_info_ms_bug_workaround_type<span_range_type>& signature,
	error_list& errors);
template void validate_autenticode_timestamp_format<vector_range_type>(
	const authenticode_signature_cms_info_ms_bug_workaround_type<vector_range_type>& signature,
	error_list& errors);
template void validate_autenticode_timestamp_format<span_range_type>(
	const authenticode_signature_cms_info_type<span_range_type>& signature,
	error_list& errors);
template void validate_autenticode_timestamp_format<vector_range_type>(
	const authenticode_signature_cms_info_type<vector_range_type>& signature,
	error_list& errors);

template void validate_autenticode_timestamp_format<span_range_type>(
	const pkcs7::attribute_map<span_range_type>& authenticated_attributes,
	error_list& errors);
template void validate_autenticode_timestamp_format<vector_range_type>(
	const pkcs7::attribute_map<vector_range_type>& authenticated_attributes,
	error_list& errors);

template void validate_autenticode_timestamp_format<span_range_type>(
	const authenticode_timestamp_signature<span_range_type>& authenticated_attributes,
	error_list& errors);
template void validate_autenticode_timestamp_format<vector_range_type>(
	const authenticode_timestamp_signature<vector_range_type>& authenticated_attributes,
	error_list& errors);

} //namespace pe_bliss::security
