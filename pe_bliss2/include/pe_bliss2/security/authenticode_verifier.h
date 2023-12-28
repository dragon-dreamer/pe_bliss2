#pragma once

#include <utility>

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/authenticode_certificate_store.h"
#include "pe_bliss2/security/authenticode_check_status.h"
#include "pe_bliss2/security/authenticode_format_validator.h"
#include "pe_bliss2/security/authenticode_loader.h"
#include "pe_bliss2/security/authenticode_page_hashes.h"
#include "pe_bliss2/security/authenticode_pkcs7.h"
#include "pe_bliss2/security/authenticode_timestamp_signature_format_validator.h"
#include "pe_bliss2/security/authenticode_timestamp_signature_verifier.h"
#include "pe_bliss2/security/authenticode_verification_options.h"
#include "pe_bliss2/security/buffer_hash.h"
#include "pe_bliss2/security/crypto_algorithms.h"
#include "pe_bliss2/security/image_hash.h"
#include "pe_bliss2/security/pkcs7/pkcs7.h"
#include "pe_bliss2/security/pkcs7/message_digest.h"
#include "pe_bliss2/security/pkcs7/pkcs7_format_validator.h"
#include "pe_bliss2/security/pkcs7/pkcs7_signature.h"
#include "pe_bliss2/security/signature_verifier.h"

#include "simple_asn1/types.h"

namespace pe_bliss::image { class image; }

namespace pe_bliss::security
{

// Requires cert_store to be set inside authenticode_check_status_base
template<typename RangeType1, typename RangeType2,
	typename RangeType3, typename RangeType4, typename RangeType5 = span_range_type>
void verify_valid_format_authenticode(
	const authenticode_pkcs7<RangeType1>& authenticode,
	const pkcs7::signer_info_ref_pkcs7<RangeType2>& signer,
	const pkcs7::attribute_map<RangeType3>& authenticated_attributes,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base<RangeType4>& result,
	const pkcs7::attribute_map<RangeType5>* unauthenticated_attributes = nullptr)
{
	auto& digest_alg = result.image_digest_alg.emplace();
	auto& digest_encryption_alg = result.digest_encryption_alg.emplace();
	if (!get_hash_and_signature_algorithms(signer,
		digest_alg, digest_encryption_alg, result.authenticode_format_errors))
	{
		return;
	}

	std::optional<authenticode_page_hashes<span_range_type>> page_hashes;
	std::optional<page_hash_options> page_hash_opts;
	std::optional<span_range_type> raw_page_hashes;
	try
	{
		page_hashes = get_page_hashes<span_range_type>(authenticode);
	}
	catch (const pe_error&)
	{
		result.page_hashes_check_errc
			= authenticode_verifier_errc::invalid_page_hash_format;
	}

	while (page_hashes)
	{
		if (!page_hashes->is_valid(digest_alg))
		{
			result.page_hashes_check_errc
				= authenticode_verifier_errc::invalid_page_hash_format;
			break;
		}

		page_hash_opts.emplace(opts.page_hash_opts).algorithm = digest_alg;
		raw_page_hashes = page_hashes->get_raw_page_hashes();
		break;
	}

	try
	{
		const auto hash_result = verify_image_hash(
			authenticode.get_image_hash(), digest_alg, instance,
			raw_page_hashes, page_hash_opts);
		result.image_hash_valid = hash_result.image_hash_valid;
		result.page_hashes_valid = hash_result.page_hashes_valid;
		if (hash_result.page_hashes_check_errc)
			result.page_hashes_check_errc = hash_result.page_hashes_check_errc;
	}
	catch (const std::exception&)
	{
		result.authenticode_format_errors.add_error(
			authenticode_verifier_errc::invalid_image_format_for_hashing);
		return;
	}

	try
	{
		result.message_digest_valid = verify_message_digest(authenticode,
			signer, authenticated_attributes);
	}
	catch (const std::exception&)
	{
		result.authenticode_format_errors.add_error(
			pkcs7::pkcs7_format_validator_errc::invalid_message_digest);
		return;
	}

	result.signature_result = verify_signature(signer, result.cert_store.value());

	if (!opts.verify_timestamp_signature)
		return;

	try
	{
		if (unauthenticated_attributes)
		{
			result.timestamp_signature_result = verify_timestamp_signature_ex<RangeType4>(
				authenticode.get_signer(0).get_encrypted_digest(),
				*unauthenticated_attributes, result.cert_store.value());
		}
		else
		{
			result.timestamp_signature_result = verify_timestamp_signature_ex<RangeType4>(
				authenticode.get_signer(0).get_encrypted_digest(),
				signer.get_unauthenticated_attributes(),
				result.cert_store.value());
		}
	}
	catch (const pe_error& e)
	{
		result.authenticode_format_errors.add_error(e.code());
	}
}

template<typename Authenticode, typename RangeType, typename RangeType2 = span_range_type>
void verify_authenticode(Authenticode&& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base<RangeType>& result,
	const pkcs7::attribute_map<RangeType2>* unauthenticated_attributes = nullptr)
{
	validate_autenticode_format(authenticode, result.authenticode_format_errors);
	if (result.authenticode_format_errors.has_errors())
		return;

	const auto& signer = authenticode.get_signer(0u);

	pkcs7::attribute_map<RangeType> authenticated_attributes;
	try
	{
		authenticated_attributes = signer.get_authenticated_attributes();
	}
	catch (const pe_error& e)
	{
		result.authenticode_format_errors.add_error(e.code());
		return;
	}

	validate_autenticode_format(authenticated_attributes,
		result.signing_time, result.authenticode_format_errors);
	if (result.authenticode_format_errors.has_errors())
		return;

	result.cert_store = build_certificate_store(
		authenticode, &result.certificate_store_warnings);
	verify_valid_format_authenticode(
		authenticode, signer, authenticated_attributes,
		instance, opts, result, unauthenticated_attributes);
	result.signature = std::forward<Authenticode>(authenticode);
}

template<typename Authenticode>
[[nodiscard]]
authenticode_check_status<typename std::remove_cvref_t<Authenticode>::range_type> verify_authenticode_full(
	Authenticode&& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts = {})
{
	using range_type = typename std::remove_cvref_t<Authenticode>::range_type;
	authenticode_check_status<range_type> result;

	const auto& content_info = authenticode.get_content_info();
	pkcs7::attribute_map<range_type> unauthenticated_attributes;
	if (content_info.data.signer_infos.size() == 1u)
	{
		const auto& signer = authenticode.get_signer(0u);
		try
		{
			unauthenticated_attributes = signer.get_unauthenticated_attributes();
		}
		catch (const pe_error& e)
		{
			result.root.authenticode_format_errors.add_error(e.code());
			return result;
		}
	}

	auto nested_signatures = load_nested_signatures<range_type>(unauthenticated_attributes);
	result.nested.reserve(nested_signatures.size());
	for (auto& nested_signature : nested_signatures)
	{
		verify_authenticode(std::move(nested_signature),
			instance, opts, result.nested.emplace_back());
	}

	verify_authenticode(std::forward<Authenticode>(authenticode), instance, opts,
		result.root, &unauthenticated_attributes);

	return result;
}

} //namespace pe_bliss::security
