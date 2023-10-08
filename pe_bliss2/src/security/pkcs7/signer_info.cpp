#include "pe_bliss2/security/pkcs7/signer_info.h"

#include <array>
#include <optional>
#include <string>
#include <system_error>

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/buffer_hash.h"
#include "pe_bliss2/security/byte_range_types.h"

#include "simple_asn1/crypto/pkcs7/spec.h"

namespace
{
struct signer_info_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "signer_info";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::pkcs7::signer_info_errc;
		switch (static_cast<pe_bliss::security::pkcs7::signer_info_errc>(ev))
		{
		case duplicate_attribute_oid:
			return "Duplicate attribute OID";
		case absent_authenticated_attributes:
			return "Absent authenticated attributes";
		default:
			return {};
		}
	}
};

const signer_info_error_category signer_info_error_category_instance;

} //namespace

namespace pe_bliss::security::pkcs7
{

std::error_code make_error_code(signer_info_errc e) noexcept
{
	return { static_cast<int>(e), signer_info_error_category_instance };
}

namespace
{
template<typename RangeType>
const auto& attributes_value(const std::optional<asn1::with_raw_data<RangeType,
	asn1::crypto::pkcs7::attributes_type<RangeType>>>& attributes)
{
	return attributes->value;
}

template<typename RangeType>
const auto& attributes_value(const std::optional<
	asn1::crypto::pkcs7::attributes_type<RangeType>>& attributes)
{
	return *attributes;
}

template<typename RangeType, typename Attributes>
attribute_map<RangeType> get_attributes(const Attributes& attributes)
{
	attribute_map<RangeType> result;

	if (!attributes)
		return result;

	result.get_map().reserve(attributes_value(attributes).size());
	for (const auto& attribute : attributes_value(attributes))
	{
		if (!result.get_map().emplace(attribute.type.container, attribute.values).second)
			throw pe_error(signer_info_errc::duplicate_attribute_oid);
	}

	return result;
}
} //namespace

template<typename RangeType, typename SignerInfoType, typename UnderlyingType>
attribute_map<RangeType> signer_info_base<RangeType, SignerInfoType, UnderlyingType>
	::get_authenticated_attributes() const
{
	return get_attributes<RangeType>(signer_info_ref_.authenticated_attributes);
}

template<typename RangeType, typename SignerInfoType, typename UnderlyingType>
attribute_map<RangeType> signer_info_base<RangeType, SignerInfoType, UnderlyingType>
	::get_unauthenticated_attributes() const
{
	return get_attributes<RangeType>(signer_info_ref_.unauthenticated_attributes);
}

template<typename RangeType, typename SignerInfoType, typename UnderlyingType>
std::vector<std::byte> signer_info_base<RangeType, SignerInfoType, UnderlyingType>
	::calculate_message_digest(std::span<const span_range_type> raw_signed_content) const
{
	return calculate_hash(get_digest_algorithm(), raw_signed_content);
}

template<typename RangeType, typename SignerInfoType, typename UnderlyingType>
std::vector<std::byte> signer_info_base<RangeType, SignerInfoType, UnderlyingType>
	::calculate_authenticated_attributes_digest() const
{
	if (!signer_info_ref_.authenticated_attributes)
		throw pe_error(signer_info_errc::absent_authenticated_attributes);

	span_range_type raw_attributes = signer_info_ref_.authenticated_attributes->raw;

	// Replace ASN.1 IMPLICIT TAGGED tag with SET_OF
	const std::array<std::byte, 1u> replaced_byte{
		static_cast<std::byte>(asn1::spec::crypto::pkcs7::unauthenticated_attributes::tag())
	};

	return calculate_hash(get_digest_algorithm(),
		std::array<span_range_type, 2u>{ replaced_byte, raw_attributes.subspan(1u) });
}

template class signer_info_base<span_range_type,
	asn1::crypto::pkcs7::signer_info<span_range_type>,
	const asn1::crypto::pkcs7::signer_info<span_range_type>&>;
template class signer_info_base<vector_range_type,
	asn1::crypto::pkcs7::signer_info<vector_range_type>,
	const asn1::crypto::pkcs7::signer_info<vector_range_type>&>;
template class signer_info_base<span_range_type,
	asn1::crypto::pkcs7::cms::signer_info<span_range_type>,
	const asn1::crypto::pkcs7::cms::signer_info<span_range_type>&>;
template class signer_info_base<vector_range_type,
	asn1::crypto::pkcs7::cms::signer_info<vector_range_type>,
	const asn1::crypto::pkcs7::cms::signer_info<vector_range_type>&>;

template class signer_info_base<span_range_type,
	asn1::crypto::pkcs7::signer_info<span_range_type>,
	asn1::crypto::pkcs7::signer_info<span_range_type>>;
template class signer_info_base<vector_range_type,
	asn1::crypto::pkcs7::signer_info<vector_range_type>,
	asn1::crypto::pkcs7::signer_info<vector_range_type>>;
template class signer_info_base<span_range_type,
	asn1::crypto::pkcs7::cms::signer_info<span_range_type>,
	asn1::crypto::pkcs7::cms::signer_info<span_range_type>>;
template class signer_info_base<vector_range_type,
	asn1::crypto::pkcs7::cms::signer_info<vector_range_type>,
	asn1::crypto::pkcs7::cms::signer_info<vector_range_type>>;

} //namespace pe_bliss::security::pkcs7
