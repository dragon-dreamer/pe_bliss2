#include "pe_bliss2/security/pkcs7/pkcs7.h"

#include <algorithm>
#include <array>
#include <string>
#include <system_error>

#include "pe_bliss2/pe_error.h"

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
		default:
			return {};
		}
	}
};

const pkcs7_error_category pkcs7_error_category_instance;

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
} //namespace

namespace pe_bliss::security::pkcs7
{

std::error_code make_error_code(pkcs7_errc e) noexcept
{
	return { static_cast<int>(e), pkcs7_error_category_instance };
}

template<typename RangeType>
digest_algorithm signer_info<RangeType>::get_digest_algorithm() const noexcept
{
	const auto& algorithm = signer_info_ref_
		.digest_algorithm.algorithm.container;

	if (std::ranges::equal(algorithm, sha256_oid))
		return digest_algorithm::sha256;
	if (std::ranges::equal(algorithm, sha1_oid))
		return digest_algorithm::sha1;
	if (std::ranges::equal(algorithm, md5_oid))
		return digest_algorithm::md5;

	return digest_algorithm::unknown;
}

template<typename RangeType>
digest_encryption_algorithm signer_info<RangeType>
	::get_digest_encryption_algorithm() const noexcept
{
	const auto& algorithm = signer_info_ref_
		.digest_encryption_algorithm.algorithm.container;

	if (std::ranges::equal(algorithm, rsa_oid))
		return digest_encryption_algorithm::rsa;
	if (std::ranges::equal(algorithm, dsa_oid))
		return digest_encryption_algorithm::dsa;

	return digest_encryption_algorithm::unknown;
}

template<typename RangeType>
attribute_map<RangeType> signer_info<RangeType>::get_authenticated_attributes() const
{
	attribute_map<RangeType> result;

	const auto& attributes = signer_info_ref_.authenticated_attributes;
	if (!attributes)
		return result;

	result.get_map().reserve(attributes->size());
	for (const auto& attribute : *attributes)
	{
		if (!result.get_map().emplace(attribute.type.container, attribute.values).second)
			throw pe_error(pkcs7_errc::duplicate_attribute_oid);
	}

	return result;
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
std::optional<span_range_type> attribute_map<RangeType>::get_message_digest() const
{
	return get_attribute(message_digest_oid);
}

template<typename RangeType>
std::optional<span_range_type> attribute_map<RangeType>::get_content_type() const
{
	return get_attribute(content_type_oid);
}

template class signer_info<span_range_type>;
template class signer_info<vector_range_type>;
template class attribute_map<span_range_type>;
template class attribute_map<vector_range_type>;

} //namespace pe_bliss::security::pkcs7
