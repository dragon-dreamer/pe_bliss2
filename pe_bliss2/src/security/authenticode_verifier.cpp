#include "pe_bliss2/security/authenticode_verifier.h"

#include <cstdint>
#include <optional>
#include <string>
#include <system_error>

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/authenticode_certificate_store.h"
#include "pe_bliss2/security/authenticode_format_validator.h"
#include "pe_bliss2/security/authenticode_loader.h"
#include "pe_bliss2/security/authenticode_page_hashes.h"
#include "pe_bliss2/security/authenticode_format_validator.h"
#include "pe_bliss2/security/authenticode_timestamp_signature_format_validator.h"
#include "pe_bliss2/security/buffer_hash.h"
#include "pe_bliss2/security/crypto_algorithms.h"
#include "pe_bliss2/security/pkcs7/message_digest.h"
#include "pe_bliss2/security/pkcs7/pkcs7_format_validator.h"
#include "pe_bliss2/security/signature_verifier.h"

namespace
{

struct authenticode_verifier_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "authenticode_verifier";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::authenticode_verifier_errc;
		switch (static_cast<pe_bliss::security::authenticode_verifier_errc>(ev))
		{
		case invalid_page_hash_format:
			return "Invalid page hash format";
		default:
			return {};
		}
	}
};

const authenticode_verifier_error_category authenticode_verifier_error_category_instance;

} //namespace

namespace pe_bliss::security
{

std::error_code make_error_code(authenticode_verifier_errc e) noexcept
{
	return { static_cast<int>(e), authenticode_verifier_error_category_instance };
}

template<typename RangeType>
void verify_valid_format_authenticode(
	const authenticode_pkcs7<RangeType>& authenticode,
	const pkcs7::signer_info_ref_pkcs7<RangeType>& signer,
	const pkcs7::attribute_map<RangeType>& authenticated_attributes,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base<RangeType>& result)
{
	auto& digest_alg = result.image_digest_alg.emplace();
	auto& digest_encryption_alg = result.digest_encryption_alg.emplace();
	if (!get_hash_and_signature_algorithms(signer,
		digest_alg, digest_encryption_alg, result.authenticode_format_errors))
	{
		return;
	}

	const auto page_hashes = pe_bliss::security::get_page_hashes<
		pe_bliss::security::span_range_type>(authenticode);
	std::optional<page_hash_options> page_hash_opts;
	std::optional<span_range_type> raw_page_hashes;
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
		if (!result.page_hashes_check_errc)
			result.page_hashes_check_errc = hash_result.page_hashes_check_errc;
		result.message_digest_valid = verify_message_digest(authenticode,
			signer, authenticated_attributes);
	}
	catch (const std::exception&)
	{
		result.authenticode_processing_error = std::current_exception();
		return;
	}

	result.signature_result = verify_signature(signer, result.cert_store.value());
}

template<typename RangeType>
void verify_authenticode(const authenticode_pkcs7<RangeType>& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base<RangeType>& result)
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
		authenticode, signer, authenticated_attributes, instance, opts, result);
}

template<typename RangeType>
authenticode_check_status<RangeType> verify_authenticode_full(
	const authenticode_pkcs7<RangeType>& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts)
{
	authenticode_check_status<RangeType> result;
	verify_authenticode(authenticode, instance, opts, result.root);
	if (!result.root)
		return result;

	const auto& signer = authenticode.get_signer(0u);
	pkcs7::attribute_map<RangeType> unauthenticated_attributes;
	try
	{
		unauthenticated_attributes = signer.get_unauthenticated_attributes();
	}
	catch (const pe_error& e)
	{
		result.root.authenticode_format_errors.add_error(e.code());
		return result;
	}

	const auto nested_signatures = load_nested_signatures<RangeType>(unauthenticated_attributes);
	result.nested.reserve(nested_signatures.size());
	for (const auto& nested_signature : nested_signatures)
		verify_authenticode(nested_signature, instance, opts, result.nested.emplace_back());

	return result;
}


template void verify_valid_format_authenticode<span_range_type>(
	const authenticode_pkcs7<span_range_type>& authenticode,
	const pkcs7::signer_info_ref_pkcs7<span_range_type>& signer,
	const pkcs7::attribute_map<span_range_type>& authenticated_attributes,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base<span_range_type>& result);
template void verify_valid_format_authenticode<vector_range_type>(
	const authenticode_pkcs7<vector_range_type>& authenticode,
	const pkcs7::signer_info_ref_pkcs7<vector_range_type>& signer,
	const pkcs7::attribute_map<vector_range_type>& authenticated_attributes,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base<vector_range_type>& result);

template void verify_authenticode<span_range_type>(
	const authenticode_pkcs7<span_range_type>& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base<span_range_type>& result);
template void verify_authenticode<vector_range_type>(
	const authenticode_pkcs7<vector_range_type>& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base<vector_range_type>& result);

template authenticode_check_status<span_range_type> verify_authenticode_full<span_range_type>(
	const authenticode_pkcs7<span_range_type>& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts);
template authenticode_check_status<vector_range_type> verify_authenticode_full<vector_range_type>(
	const authenticode_pkcs7<vector_range_type>& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts);

} //namespace pe_bliss::security
