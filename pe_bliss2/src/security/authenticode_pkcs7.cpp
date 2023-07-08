#include "pe_bliss2/security/authenticode_pkcs7.h"

#include <algorithm>
#include <array>
#include <string>

#include "pe_bliss2/pe_error.h"

namespace
{
constexpr std::array sha1_oid {
	1u, 3u, 14u, 3u, 2u, 26u
};

constexpr std::array sha256_oid {
	2u, 16u, 840u, 1u, 101u, 3u, 4u, 2u, 1u
};

constexpr std::array md5_oid {
	1u, 2u, 840u, 113549u, 2u, 5u
};

constexpr std::array rsa_oid {
	1u, 2u, 840u, 113549u, 1u, 1u, 1u
};

constexpr std::array dsa_oid {
	1u, 2u, 840u, 10040u, 4u, 1u
};

constexpr std::array message_digest_oid {
	1u, 2u, 840u, 113549u, 1u, 9u, 4u
};

constexpr std::array content_type_oid {
	1u, 2u, 840u, 113549u, 1u, 9u, 3u
};

constexpr std::array spc_sp_opus_info_objid {
	1u, 3u, 6u, 1u, 4u, 1u, 311u, 2u, 1u, 12u
};

struct authenticode_pkcs7_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "authenticode_pkcs7";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::authenticode_pkcs7_errc;
		switch (static_cast<pe_bliss::security::authenticode_pkcs7_errc>(ev))
		{
		case duplicate_attribute_oid:
			return "Duplicate attribute OID";
		case absent_attribute_value:
			return "Absent attribute value";
		case multiple_attribute_values:
			return "Multiple attribute values";
		default:
			return {};
		}
	}
};

const authenticode_pkcs7_error_category authenticode_pkcs7_error_category_instance;
} //namespace

namespace pe_bliss::security
{

std::error_code make_error_code(authenticode_pkcs7_errc e) noexcept
{
	return { static_cast<int>(e), authenticode_pkcs7_error_category_instance };
}

template<typename RangeType>
authenticode_digest_algorithm authenticode_pkcs7<RangeType>
	::get_digest_algorithm() const noexcept
{
	if (content_info_.data.signer_infos.empty())
		return authenticode_digest_algorithm::unknown;

	const auto& algorithm = content_info_.data.signer_infos[0]
		.digest_algorithm.algorithm.container;

	if (std::ranges::equal(algorithm, sha256_oid))
		return authenticode_digest_algorithm::sha256;
	if (std::ranges::equal(algorithm, sha1_oid))
		return authenticode_digest_algorithm::sha1;
	if (std::ranges::equal(algorithm, md5_oid))
		return authenticode_digest_algorithm::md5;

	return authenticode_digest_algorithm::unknown;
}

template<typename RangeType>
authenticode_digest_encryption_algorithm authenticode_pkcs7<RangeType>
	::get_digest_encryption_algorithm() const noexcept
{
	if (content_info_.data.signer_infos.empty())
		return authenticode_digest_encryption_algorithm::unknown;

	const auto& algorithm = content_info_.data.signer_infos[0]
		.digest_encryption_algorithm.algorithm.container;

	if (std::ranges::equal(algorithm, rsa_oid))
		return authenticode_digest_encryption_algorithm::rsa;
	if (std::ranges::equal(algorithm, dsa_oid))
		return authenticode_digest_encryption_algorithm::dsa;

	return authenticode_digest_encryption_algorithm::unknown;
}

template<typename RangeType>
authenticode_attribute_map<RangeType> authenticode_pkcs7<RangeType>
	::get_authenticated_attributes() const
{
	authenticode_attribute_map<RangeType> result;

	if (content_info_.data.signer_infos.empty())
		return result;

	const auto& attributes = content_info_.data.signer_infos[0].authenticated_attributes;
	if (!attributes)
		return result;

	result.get_map().reserve(attributes->size());
	for (const auto& attribute : *attributes)
	{
		if (!result.get_map().emplace(attribute.type.container, attribute.values).second)
			throw pe_error(authenticode_pkcs7_errc::duplicate_attribute_oid);
	}

	return result;
}

template<typename RangeType>
authenticode_span_range_type authenticode_pkcs7<RangeType>::get_image_hash() const noexcept
{
	return content_info_.data.content_info.value.content.digest.digest;
}

template<typename RangeType>
std::optional<authenticode_span_range_type> authenticode_attribute_map<RangeType>::get_attribute(
	std::span<const std::uint32_t> oid) const
{
	std::optional<authenticode_span_range_type> result;

	auto it = map_.find(oid);
	if (it != map_.cend())
	{
		if (it->second.get().empty())
			throw pe_error(authenticode_pkcs7_errc::absent_attribute_value);
		if (it->second.get().size() != 1u)
			throw pe_error(authenticode_pkcs7_errc::multiple_attribute_values);

		result.emplace(it->second.get()[0]);
	}

	return result;
}

template<typename RangeType>
std::optional<authenticode_span_range_type> authenticode_attribute_map<RangeType>::get_message_digest() const
{
	return get_attribute(message_digest_oid);
}

template<typename RangeType>
std::optional<authenticode_span_range_type> authenticode_attribute_map<RangeType>::get_content_type() const
{
	return get_attribute(content_type_oid);
}

template<typename RangeType>
std::optional<authenticode_span_range_type> authenticode_attribute_map<RangeType>::get_spc_sp_opus_info() const
{
	return get_attribute(spc_sp_opus_info_objid);
}

template class authenticode_pkcs7<authenticode_span_range_type>;
template class authenticode_pkcs7<authenticode_vector_range_type>;
template class authenticode_attribute_map<authenticode_span_range_type>;
template class authenticode_attribute_map<authenticode_vector_range_type>;

} //namespace pe_bliss::security
