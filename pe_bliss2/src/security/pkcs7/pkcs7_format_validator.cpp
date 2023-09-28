#include "pe_bliss2/security/pkcs7/pkcs7_format_validator.h"

#include <string>
#include <system_error>

#include "pe_bliss2/pe_error.h"

#include "simple_asn1/crypto/crypto_common_spec.h"
#include "simple_asn1/der_decode.h"
#include "simple_asn1/spec.h"

namespace
{

struct pkcs7_format_validator_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "pkcs7_format_validator";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::pkcs7::pkcs7_format_validator_errc;
		switch (static_cast<pe_bliss::security::pkcs7::pkcs7_format_validator_errc>(ev))
		{
		case invalid_signed_data_oid:
			return "Invalid signed data OID";
		case invalid_signed_data_version:
			return "Invalid signed data version (must be 1)";
		case invalid_signer_count:
			return "Invalid signer count (must be 1)";
		case non_matching_digest_algorithm:
			return "Digest algorithm of signed data does not match digest algorithm of signer info";
		case invalid_signer_info_version:
			return "Invalid signer info version (must be 1)";
		case absent_message_digest:
			return "Absent message digest attribute";
		case invalid_message_digest:
			return "Invalid message digest attribute";
		case absent_content_type:
			return "Absent content type attribute";
		case invalid_content_type:
			return "Invalid content type attribute value";
		case invalid_signing_time:
			return "Invalid signing time";
		default:
			return {};
		}
	}
};

const pkcs7_format_validator_error_category pkcs7_format_validator_error_category_instance;

} //namespace

namespace pe_bliss::security::pkcs7
{

std::error_code make_error_code(pkcs7_format_validator_errc e) noexcept
{
	return { static_cast<int>(e), pkcs7_format_validator_error_category_instance };
}

template<typename RangeType>
void validate_authenticated_attributes(
	const pe_bliss::security::pkcs7::attribute_map<RangeType>& authenticated_attributes,
	std::optional<asn1::utc_time>& signing_time,
	std::optional<asn1::crypto::object_identifier_type>& content_type,
	error_list& errors)
{
	try
	{
		if (!authenticated_attributes.get_message_digest())
			errors.add_error(pkcs7_format_validator_errc::absent_message_digest);
	}
	catch (const pe_error&)
	{
		errors.add_error(pkcs7_format_validator_errc::invalid_message_digest);
	}

	try
	{
		auto content_type_attr = authenticated_attributes.get_content_type();
		if (!content_type_attr)
		{
			errors.add_error(pkcs7_format_validator_errc::absent_content_type);
		}
		else
		{
			content_type = asn1::der::decode<asn1::crypto::object_identifier_type,
				asn1::spec::object_identifier<>>(
					content_type_attr->begin(), content_type_attr->end());
		}
	}
	catch (const pe_error&)
	{
		errors.add_error(pkcs7_format_validator_errc::invalid_content_type);
	}
	catch (const asn1::parse_error&)
	{
		errors.add_error(pkcs7_format_validator_errc::invalid_content_type);
	}

	try
	{
		if (auto signing_time_attr = authenticated_attributes.get_signing_time(); signing_time_attr)
		{
			signing_time = asn1::der::decode<asn1::utc_time, asn1::spec::utc_time<>>(
				signing_time_attr->begin(), signing_time_attr->end());
		}
	}
	catch (const pe_error&)
	{
		errors.add_error(pkcs7_format_validator_errc::invalid_signing_time);
	}
	catch (const asn1::parse_error&)
	{
		errors.add_error(pkcs7_format_validator_errc::invalid_signing_time);
	}
}

template void validate_authenticated_attributes<span_range_type>(
	const pe_bliss::security::pkcs7::attribute_map<span_range_type>& authenticated_attributes,
	std::optional<asn1::utc_time>& signing_time,
	std::optional<asn1::crypto::object_identifier_type>& content_type,
	error_list& errors);
template void validate_authenticated_attributes<vector_range_type>(
	const pe_bliss::security::pkcs7::attribute_map<vector_range_type>& authenticated_attributes,
	std::optional<asn1::utc_time>& signing_time,
	std::optional<asn1::crypto::object_identifier_type>& content_type,
	error_list& errors);

} //namespace pe_bliss::security::pkcs7
