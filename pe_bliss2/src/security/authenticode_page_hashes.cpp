#include "pe_bliss2/security/authenticode_page_hashes.h"

#include <algorithm>
#include <cstddef>
#include <variant>

#include "pe_bliss2/security/byte_range_types.h"

#include "simple_asn1/crypto/pkcs7/authenticode/oids.h"
#include "simple_asn1/crypto/pkcs7/authenticode/spec.h"
#include "simple_asn1/crypto/pkcs7/authenticode/types.h"
#include "simple_asn1/der_decode.h"

#include "utilities/variant_helpers.h"

namespace pe_bliss::security
{

namespace
{
template<typename RangeType>
bool is_valid_oid(const RangeType& range) noexcept
{
	return std::ranges::equal(range,
		asn1::crypto::pkcs7::authenticode::oid_spc_page_hash_v1)
	|| std::ranges::equal(range,
		asn1::crypto::pkcs7::authenticode::oid_spc_page_hash_v2);
}

template<typename PageHashes>
bool is_valid_impl(const PageHashes& hashes)
{
	return hashes.size() == 1u
		&& hashes[0].hashes.size() == 1u
		&& is_valid_oid(hashes[0].type.container);
}

std::size_t get_hash_digest_size(digest_algorithm algorithm) noexcept
{
	switch (algorithm)
	{
	case digest_algorithm::sha1:
		return 20u;
	case digest_algorithm::sha256:
		return 32u;
	case digest_algorithm::sha384:
		return 48u;
	case digest_algorithm::sha512:
		return 64u;
	default:
		return 0u;
	}
}
} //namespace

template<typename RangeType>
bool authenticode_page_hashes<RangeType>::is_valid(digest_algorithm alg) const noexcept
{
	if (!is_valid_impl(page_hashes_))
		return false;

	const auto size = page_hashes_[0].hashes[0].size();
	return size && (size % (get_hash_digest_size(alg) + 4u)) == 0u;
}

template<typename RangeType>
std::optional<span_range_type> authenticode_page_hashes<RangeType>
	::get_raw_page_hashes() const noexcept
{
	if (page_hashes_.size() == 1u
		&& page_hashes_[0].hashes.size() == 1u)
	{
		return page_hashes_[0].hashes[0];
	}
	return {};
}

template<typename TargetRangeType, typename RangeType>
std::optional<authenticode_page_hashes<TargetRangeType>> get_page_hashes(
	const authenticode_pkcs7<RangeType>& authenticode)
{
	std::optional<authenticode_page_hashes<TargetRangeType>> result;

	const auto& file = authenticode.get_content_info()
		.data.content_info.content.type_value.value.value.file;
	if (!file)
		return result;

	std::visit(utilities::overloaded{
		[](const auto&) {},
		[&result](
			const asn1::crypto::pkcs7::authenticode::spc_serialized_object<RangeType>& obj) {
			if (std::ranges::equal(obj.class_id,
				asn1::crypto::pkcs7::authenticode::page_hashes_class_id))
			{
				auto& hashes = result.emplace();
				//TODO: check return value
				asn1::der::decode<asn1::spec::crypto::pkcs7::authenticode::spc_attribute_page_hashes>(
					obj.serialized_data.begin(), obj.serialized_data.end(),
					hashes.get_underlying_struct());
			}
		}
	}, *file);

	return result;
}

template std::optional<authenticode_page_hashes<span_range_type>> get_page_hashes<
	span_range_type, span_range_type>(const authenticode_pkcs7<span_range_type>& authenticode);
template std::optional<authenticode_page_hashes<span_range_type>> get_page_hashes<
	span_range_type, vector_range_type>(const authenticode_pkcs7<vector_range_type>& authenticode);
template std::optional<authenticode_page_hashes<vector_range_type>> get_page_hashes<
	vector_range_type, span_range_type>(const authenticode_pkcs7<span_range_type>& authenticode);
template std::optional<authenticode_page_hashes<vector_range_type>> get_page_hashes<
	vector_range_type, vector_range_type>(const authenticode_pkcs7<vector_range_type>& authenticode);

template class authenticode_page_hashes<span_range_type>;
template class authenticode_page_hashes<vector_range_type>;

} //namespace pe_bliss::security
