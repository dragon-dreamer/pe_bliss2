#include "pe_bliss2/security/pkcs7/pkcs7.h"

#include <algorithm>
#include <array>
#include <string>
#include <system_error>

#include "buffers/input_memory_buffer.h"

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/buffer_hash.h"

#include "simple_asn1/der_decode.h"
#include "simple_asn1/spec.h"

#include "simple_asn1/crypto/pkcs7/oids.h"
#include "simple_asn1/crypto/pkcs7/spec.h"

namespace
{
struct pkcs7_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "pkcs7";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::pkcs7::pkcs7_errc;
		switch (static_cast<pe_bliss::security::pkcs7::pkcs7_errc>(ev))
		{
		case duplicate_attribute_oid:
			return "Duplicate attribute OID";
		case absent_attribute_value:
			return "Absent attribute value";
		case multiple_attribute_values:
			return "Multiple attribute values";
		case absent_authenticated_attributes:
			return "Absent authenticated attributes";
		default:
			return {};
		}
	}
};

const pkcs7_error_category pkcs7_error_category_instance;

} //namespace

namespace pe_bliss::security::pkcs7
{

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
			throw pe_error(pkcs7_errc::duplicate_attribute_oid);
	}

	return result;
}
} //namespace

namespace impl
{
span_range_type decode_octet_string(span_range_type source)
{
	return asn1::der::decode<span_range_type, asn1::spec::octet_string<>>(
		source.begin(), source.end());
}
} //namespace impl

std::error_code make_error_code(pkcs7_errc e) noexcept
{
	return { static_cast<int>(e), pkcs7_error_category_instance };
}

template<typename RangeType>
attribute_map<RangeType> signer_info_ref<RangeType>::get_authenticated_attributes() const
{
	return get_attributes<RangeType>(signer_info_ref_.authenticated_attributes);
}

template<typename RangeType>
attribute_map<RangeType> signer_info_ref<RangeType>::get_unauthenticated_attributes() const
{
	return get_attributes<RangeType>(signer_info_ref_.unauthenticated_attributes);
}

template<typename RangeType>
std::vector<std::byte> signer_info_ref<RangeType>::calculate_message_digest(
	std::span<const span_range_type> raw_signed_content) const
{
	return calculate_hash(get_digest_algorithm(), raw_signed_content);
}

template<typename RangeType>
std::vector<std::byte> signer_info_ref<RangeType>::calculate_authenticated_attributes_digest() const
{
	if (!signer_info_ref_.authenticated_attributes)
		throw pe_error(pkcs7_errc::absent_authenticated_attributes);

	span_range_type raw_attributes = signer_info_ref_.authenticated_attributes->raw;

	// Replace ASN.1 IMPLICIT TAGGED tag with SET_OF
	const std::array<std::byte, 1u> replaced_byte{
		static_cast<std::byte>(asn1::spec::crypto::pkcs7::unauthenticated_attributes::tag())
	};

	return calculate_hash(get_digest_algorithm(),
		std::array<span_range_type, 2u>{ replaced_byte, raw_attributes.subspan(1u) });
}

template<typename RangeType>
std::optional<span_range_type> attribute_map<RangeType>::get_attribute(
	std::span<const std::uint32_t> oid) const
{
	std::optional<span_range_type> result;

	auto it = map_.find(oid);
	if (it != map_.cend())
	{
		if (it->second.get().empty())
			throw pe_error(pkcs7_errc::absent_attribute_value);
		if (it->second.get().size() != 1u)
			throw pe_error(pkcs7_errc::multiple_attribute_values);

		result.emplace(it->second.get()[0]);
	}

	return result;
}

template<typename RangeType>
std::vector<span_range_type> attribute_map<RangeType>::get_attributes(
	std::span<const std::uint32_t> oid) const
{
	std::vector<span_range_type> result;

	auto it = map_.find(oid);
	if (it != map_.cend())
	{
		if (it->second.get().empty())
			throw pe_error(pkcs7_errc::absent_attribute_value);

		result.reserve(it->second.get().size());
		for (const auto& value : it->second.get())
			result.emplace_back(value);
	}

	return result;
}

template<typename RangeType>
std::optional<span_range_type> attribute_map<RangeType>::get_message_digest() const
{
	return get_attribute(asn1::crypto::pkcs7::oid_message_digest);
}

template<typename RangeType>
std::optional<span_range_type> attribute_map<RangeType>::get_content_type() const
{
	return get_attribute(asn1::crypto::pkcs7::oid_content_type);
}

template class signer_info_ref<span_range_type>;
template class signer_info_ref<vector_range_type>;
template class attribute_map<span_range_type>;
template class attribute_map<vector_range_type>;

} //namespace pe_bliss::security::pkcs7
