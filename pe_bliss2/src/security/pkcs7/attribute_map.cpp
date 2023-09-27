#include "pe_bliss2/security/pkcs7/attribute_map.h"

#include <string>
#include <system_error>

#include "simple_asn1/crypto/pkcs7/oids.h"

#include "pe_bliss2/pe_error.h"

namespace
{
struct attribute_map_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "attribute_map";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::pkcs7::attribute_map_errc;
		switch (static_cast<pe_bliss::security::pkcs7::attribute_map_errc>(ev))
		{
		case absent_attribute_value:
			return "Absent attribute value";
		case multiple_attribute_values:
			return "Multiple attribute values";
		default:
			return {};
		}
	}
};

const attribute_map_error_category attribute_map_error_category_instance;

} //namespace

namespace pe_bliss::security::pkcs7
{

std::error_code make_error_code(attribute_map_errc e) noexcept
{
	return { static_cast<int>(e), attribute_map_error_category_instance };
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
			throw pe_error(attribute_map_errc::absent_attribute_value);
		if (it->second.get().size() != 1u)
			throw pe_error(attribute_map_errc::multiple_attribute_values);

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
			throw pe_error(attribute_map_errc::absent_attribute_value);

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

template<typename RangeType>
std::optional<span_range_type> attribute_map<RangeType>::get_signing_time() const
{
	return get_attribute(asn1::crypto::pkcs7::oid_signing_time);
}

template class attribute_map<span_range_type>;
template class attribute_map<vector_range_type>;

} //namespace pe_bliss::security::pkcs7
