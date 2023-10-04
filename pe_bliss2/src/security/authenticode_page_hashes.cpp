#include "pe_bliss2/security/authenticode_page_hashes.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <system_error>
#include <variant>

#include "pe_bliss2/security/asn1_decode_helper.h"
#include "pe_bliss2/security/byte_range_types.h"

#include "simple_asn1/crypto/pkcs7/authenticode/oids.h"
#include "simple_asn1/crypto/pkcs7/authenticode/spec.h"
#include "simple_asn1/crypto/pkcs7/authenticode/types.h"

#include "utilities/variant_helpers.h"

namespace
{
struct authenticode_page_hashes_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "authenticode_page_hashes";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::authenticode_page_hashes_errc;
		switch (static_cast<pe_bliss::security::authenticode_page_hashes_errc>(ev))
		{
		case invalid_authenticode_page_hashes_asn1_der:
			return "Invalid authenticode page hashes ASN.1 DER format";
		default:
			return {};
		}
	}
};

const authenticode_page_hashes_error_category authenticode_page_hashes_error_category_instance;
} //namespace

namespace pe_bliss::security
{
std::error_code make_error_code(authenticode_page_hashes_errc e) noexcept
{
	return { static_cast<int>(e), authenticode_page_hashes_error_category_instance };
}

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
	case digest_algorithm::md5:
		return 16u;
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

	const auto hash_digest_size = get_hash_digest_size(alg);
	if (!hash_digest_size)
		return false;

	const auto size = page_hashes_[0].hashes[0].size();
	return size && (size % (hash_digest_size + 4u)) == 0u;
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
				decode_asn1_check_tail<
					authenticode_page_hashes_errc::invalid_authenticode_page_hashes_asn1_der,
					asn1::spec::crypto::pkcs7::authenticode::spc_attribute_page_hashes>(
						obj.serialized_data, result.emplace().get_underlying_struct());
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
