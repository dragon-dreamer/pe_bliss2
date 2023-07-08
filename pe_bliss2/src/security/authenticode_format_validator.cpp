#include "pe_bliss2/security/authenticode_format_validator.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>

#include "pe_bliss2/pe_error.h"

#include "simple_asn1/crypto/crypto_common_types.h"
#include "simple_asn1/der_decode.h"
#include "simple_asn1/spec.h"

namespace
{
constexpr std::array signed_data_oid {
	1u, 2u, 840u, 113549u, 1u, 7u, 2u
};

constexpr std::array spc_indirect_data_objid {
	1u, 3u, 6u, 1u, 4u, 1u, 311u, 2u, 1u, 4u
};

constexpr std::array spc_pe_image_dataobj {
	1u, 3u, 6u, 1u, 4u, 1u, 311u, 2u, 1u, 15u
};

//TODO
//1.3.6.1.4.1.311.3.3.1 - Timestamping signature(Ms-CounterSign)
//PKCS #9 v1 countersignature
//The timestamp certificate chain—including the root certificate—is added to the PKCS #7 SignedData certificates structure, although the root certificate is not required
//This is an unsigned attribute in an Authenticode signature that contains a PKCS#7
//that timestamps the Portable Executable(PE) file signed by the Authenticode signature.
//The following is the timestamp attribute's OID type:
//szOID_RSA_counterSign 1.2.840.113549.1.9.6

constexpr std::uint32_t signed_data_version = 1u;
constexpr std::uint32_t signer_info_version = 1u;

struct authenticode_format_validator_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "authenticode_format_validator";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::authenticode_format_validator_errc;
		switch (static_cast<pe_bliss::security::authenticode_format_validator_errc>(ev))
		{
		case invalid_signed_data_oid:
			return "Invalid signed data OID";
		case invalid_signed_data_version:
			return "Invalid signed data version (must be 1)";
		case invalid_signer_count:
			return "Invalid signer count (must be 1)";
		case non_matching_digest_algorithm:
			return "Digest algorithm of signed data does not match digest algorithm of signer info";
		case non_matching_type_value_digest_algorithm:
			return "Digest algorithm of signer info does not match digest algorithm of DigestInfo";
		case invalid_content_info_oid:
			return "Invalid content info OID";
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
		case invalid_type_value_type:
			return "Invalid SpcAttributeTypeAndOptionalValue type";
		default:
			return {};
		}
	}
};

const authenticode_format_validator_error_category authenticode_format_validator_error_category_instance;

} //namespace

namespace pe_bliss::security
{

std::error_code make_error_code(authenticode_format_validator_errc e) noexcept
{
	return { static_cast<int>(e), authenticode_format_validator_error_category_instance };
}

bool algorithm_id_equals(const asn1::crypto::algorithm_identifier<authenticode_vector_range_type>& l,
	const asn1::crypto::algorithm_identifier<authenticode_vector_range_type>& r)
{
	return l.algorithm == r.algorithm
		&& l.parameters == r.parameters;
}

bool algorithm_id_equals(const asn1::crypto::algorithm_identifier<authenticode_span_range_type>& l,
	const asn1::crypto::algorithm_identifier<authenticode_span_range_type>& r)
{
	if (l.algorithm != r.algorithm)
		return false;

	if (l.parameters.has_value() != r.parameters.has_value())
		return false;

	if (l.parameters.has_value())
		return std::ranges::equal(*l.parameters, *r.parameters);

	return true;
}

template<typename RangeType>
void validate(const authenticode_pkcs7<RangeType>& signature,
	const authenticode_attribute_map<RangeType> authenticated_attributes,
	error_list& errors)
{
	const auto& content_info = signature.get_content_info();
	if (!std::ranges::equal(content_info.content_type.container, signed_data_oid))
		errors.add_error(authenticode_format_validator_errc::invalid_signed_data_oid);
	
	if (content_info.data.version != signed_data_version)
		errors.add_error(authenticode_format_validator_errc::invalid_signed_data_version);

	if (content_info.data.digest_algorithms.size() != 1u
		|| content_info.data.signer_infos.size() != 1u)
	{
		errors.add_error(authenticode_format_validator_errc::invalid_signer_count);
	}

	const auto& signer_info = content_info.data.signer_infos[0];
	if (!algorithm_id_equals(content_info.data.digest_algorithms[0],
		signer_info.digest_algorithm))
	{
		errors.add_error(authenticode_format_validator_errc::non_matching_digest_algorithm);
	}

	const auto& signed_data_content_info = content_info.data.content_info;
	if (!std::ranges::equal(signed_data_content_info.content_type.container, spc_indirect_data_objid))
		errors.add_error(authenticode_format_validator_errc::invalid_content_info_oid);

	if (signer_info.version != signer_info_version)
		errors.add_error(authenticode_format_validator_errc::invalid_signer_info_version);

	try
	{
		if (!authenticated_attributes.get_message_digest())
			errors.add_error(authenticode_format_validator_errc::absent_message_digest);
	}
	catch (const pe_error&)
	{
		errors.add_error(authenticode_format_validator_errc::invalid_message_digest);
	}

	try
	{
		auto content_type = authenticated_attributes.get_content_type();
		if (!content_type)
		{
			errors.add_error(authenticode_format_validator_errc::absent_content_type);
		}
		else
		{
			auto decoded_content_type = asn1::der::decode<asn1::crypto::object_identifier_type,
				asn1::spec::object_identifier<>>(content_type->begin(), content_type->end());
			if (!std::ranges::equal(decoded_content_type.container, spc_indirect_data_objid))
				errors.add_error(authenticode_format_validator_errc::invalid_content_type);
		}
	}
	catch (const pe_error&)
	{
		errors.add_error(authenticode_format_validator_errc::invalid_content_type);
	}
	catch (const asn1::parse_error&)
	{
		errors.add_error(authenticode_format_validator_errc::invalid_content_type);
	}

	const auto& type_value = signed_data_content_info.content.type_value;
	if (!std::ranges::equal(type_value.type.container, spc_pe_image_dataobj))
		errors.add_error(authenticode_format_validator_errc::invalid_type_value_type);

	if (!algorithm_id_equals(signed_data_content_info.content.digest.digest_algorithm,
		signer_info.digest_algorithm))
	{
		errors.add_error(authenticode_format_validator_errc::non_matching_type_value_digest_algorithm);
	}
}

template void validate<authenticode_span_range_type>(
	const authenticode_pkcs7<authenticode_span_range_type>& signature,
	const authenticode_attribute_map<authenticode_span_range_type> authenticated_attributes,
	error_list& errors);
template void validate<authenticode_vector_range_type>(
	const authenticode_pkcs7<authenticode_vector_range_type>& signature,
	const authenticode_attribute_map<authenticode_vector_range_type> authenticated_attributes,
	error_list& errors);

} //namespace pe_bliss::security
