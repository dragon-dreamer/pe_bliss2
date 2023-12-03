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
		case unsupported_digest_algorithm:
			return "Unsupported digest (hash) algorithm";
		case unsupported_digest_encryption_algorithm:
			return "Unable digest encryption alrogithm";
		case invalid_page_hash_format:
			return "Invalid page hash format";
		case signature_hash_and_digest_algorithm_mismatch:
			return "Signature algorithm includes a hash algorithm ID"
				" which does not match the signed specified hash algorithm";
		default:
			return {};
		}
	}
};

const authenticode_verifier_error_category authenticode_verifier_error_category_instance;

} //namespace

namespace pe_bliss::security
{

namespace
{

template<typename Signer>
bool get_hash_and_signature_algorithms(
	const Signer& signer,
	digest_algorithm& digest_alg,
	digest_encryption_algorithm& digest_encryption_alg,
	error_list& errors)
{
	digest_alg = signer.get_digest_algorithm();
	auto [signature_alg, digest_alg_from_encryption]
		= signer.get_digest_encryption_algorithm();
	if (digest_alg == digest_algorithm::unknown)
	{
		errors.add_error(authenticode_verifier_errc::unsupported_digest_algorithm);
		return false;
	}

	if (digest_encryption_alg == digest_encryption_algorithm::unknown)
	{
		errors.add_error(authenticode_verifier_errc::unsupported_digest_encryption_algorithm);
		return false;
	}

	if (digest_alg_from_encryption && digest_alg_from_encryption != digest_alg)
	{
		errors.add_error(
			authenticode_verifier_errc::signature_hash_and_digest_algorithm_mismatch);
		return false;
	}
	digest_encryption_alg = signature_alg;
	return true;
}

} //namespace

std::error_code make_error_code(authenticode_verifier_errc e) noexcept
{
	return { static_cast<int>(e), authenticode_verifier_error_category_instance };
}

template<typename Signature, typename Signer, typename RangeType>
bool verify_message_digest(const Signature& signature,
	const Signer& signer,
	const pkcs7::attribute_map<RangeType>& authenticated_attributes)
{
	const auto message_digest = pkcs7::calculate_message_digest(signature, signer);
	return pkcs7::verify_message_digest_attribute(message_digest,
		authenticated_attributes);
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

template<typename RangeType>
timestamp_signature_check_status<RangeType> verify_timestamp_signature(
	const RangeType& authenticode_encrypted_digest,
	const pkcs7::signer_info_ref_pkcs7<RangeType>& timestamp_signer,
	const pkcs7::attribute_map<RangeType>& timestamp_authenticated_attributes,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>>& cert_store)
{
	timestamp_signature_check_status<RangeType> result;

	auto& digest_alg = result.digest_alg.emplace();
	auto& digest_encryption_alg = result.digest_encryption_alg.emplace();
	if (!get_hash_and_signature_algorithms(timestamp_signer,
		digest_alg, digest_encryption_alg, result.authenticode_format_errors))
	{
		return result;
	}

	std::optional<asn1::crypto::object_identifier_type> content_type;
	std::optional<asn1::utc_time> signing_time;
	pkcs7::validate_authenticated_attributes(timestamp_authenticated_attributes,
		signing_time, content_type, result.authenticode_format_errors);
	if (signing_time)
		result.signing_time = *signing_time;

	const auto message_digest = timestamp_signer.calculate_message_digest(
		std::array<span_range_type, 1u>{ authenticode_encrypted_digest });
	result.hash_valid = pkcs7::verify_message_digest_attribute(message_digest,
		timestamp_authenticated_attributes);

	result.signature_result = verify_signature(timestamp_signer,
		cert_store);
	return result;
}

template<typename RangeType>
timestamp_signature_check_status<RangeType> verify_timestamp_signature(
	const RangeType& authenticode_encrypted_digest,
	const authenticode_signature_cms_info_ms_bug_workaround_type<RangeType>& signature)
{
	static constexpr std::int32_t cms_info_version = 3u;
	timestamp_signature_check_status<RangeType> result;

	validate_autenticode_timestamp_format(signature, result.authenticode_format_errors);
	if (result.authenticode_format_errors.has_errors())
		return result;

	const auto& signer = signature.get_signer(0u);

	pkcs7::attribute_map<RangeType> authenticated_attributes;
	try
	{
		authenticated_attributes = signer.get_authenticated_attributes();
	}
	catch (const pe_error& e)
	{
		result.authenticode_format_errors.add_error(e.code());
		return result;
	}

	validate_autenticode_timestamp_format(authenticated_attributes,
		result.authenticode_format_errors);
	if (result.authenticode_format_errors.has_errors())
		return result;

	result.cert_store = build_certificate_store<RangeType>(
		signature, &result.certificate_store_warnings);

	verify_valid_format_timestamp_signature(
		signature, signer, authenticated_attributes,
		authenticode_encrypted_digest,
		*result.cert_store, result);

	// signature.data.content_info.tsa points to attribute_certificate_v2_type
	result.signing_time = signature.get_content_info().data.content_info.info.value.gen_time;
	return result;
}

template<typename RangeType>
void verify_valid_format_timestamp_signature(
	const authenticode_signature_cms_info_ms_bug_workaround_type<RangeType>& signature,
	const pkcs7::signer_info_ref_cms<RangeType>& signer,
	const pkcs7::attribute_map<RangeType>& authenticated_attributes,
	const RangeType& authenticode_encrypted_digest,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>>& cert_store,
	timestamp_signature_check_status<RangeType>& result)
{
	auto& digest_alg = result.digest_alg.emplace();
	auto& digest_encryption_alg = result.digest_encryption_alg.emplace();
	if (!get_hash_and_signature_algorithms(signer,
		digest_alg, digest_encryption_alg, result.authenticode_format_errors))
	{
		return;
	}

	const auto& imprint = signature.get_content_info()
		.data.content_info.info.value.imprint;
	auto& imprint_digest_algorithm = result.imprint_digest_alg.emplace();
	imprint_digest_algorithm = get_digest_algorithm(imprint.hash_algorithm.algorithm.container);
	if (imprint_digest_algorithm == digest_algorithm::unknown)
	{
		return;
	}

	try
	{
		auto calculated_hash = calculate_hash(imprint_digest_algorithm,
			std::array<span_range_type, 1u>{ authenticode_encrypted_digest });
		result.hash_valid = std::ranges::equal(calculated_hash, imprint.hashed_message);
		result.message_digest_valid = verify_message_digest(signature,
			signer, authenticated_attributes);
	}
	catch (const std::exception&)
	{
		result.authenticode_processing_error = std::current_exception();
		return;
	}

	result.signature_result = verify_signature(signer, cert_store);
}

template timestamp_signature_check_status<span_range_type>
verify_timestamp_signature<span_range_type>(
	const span_range_type& authenticode_encrypted_digest,
	const authenticode_signature_cms_info_ms_bug_workaround_type<span_range_type>& signature);
template timestamp_signature_check_status<vector_range_type>
verify_timestamp_signature<vector_range_type>(
	const vector_range_type& authenticode_encrypted_digest,
	const authenticode_signature_cms_info_ms_bug_workaround_type<vector_range_type>& signature);

template timestamp_signature_check_status<span_range_type>
verify_timestamp_signature<span_range_type>(
	const span_range_type& authenticode_encrypted_digest,
	const pkcs7::signer_info_ref_pkcs7<span_range_type>& timestamp_signer,
	const pkcs7::attribute_map<span_range_type>& timestamp_authenticated_attributes,
	const x509::x509_certificate_store<x509::x509_certificate_ref<span_range_type>>& cert_store);
template timestamp_signature_check_status<vector_range_type>
verify_timestamp_signature<vector_range_type>(
	const vector_range_type& authenticode_encrypted_digest,
	const pkcs7::signer_info_ref_pkcs7<vector_range_type>& timestamp_signer,
	const pkcs7::attribute_map<vector_range_type>& timestamp_authenticated_attributes,
	const x509::x509_certificate_store<x509::x509_certificate_ref<vector_range_type>>& cert_store);

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

template void verify_valid_format_timestamp_signature<span_range_type>(
	const authenticode_signature_cms_info_ms_bug_workaround_type<span_range_type>& signature,
	const pkcs7::signer_info_ref_cms<span_range_type>& signer,
	const pkcs7::attribute_map<span_range_type>& authenticated_attributes,
	const span_range_type& authenticode_encrypted_digest,
	const x509::x509_certificate_store<x509::x509_certificate_ref<span_range_type>>& cert_store,
	timestamp_signature_check_status<span_range_type>& result);
template void verify_valid_format_timestamp_signature<vector_range_type>(
	const authenticode_signature_cms_info_ms_bug_workaround_type<vector_range_type>& signature,
	const pkcs7::signer_info_ref_cms<vector_range_type>& signer,
	const pkcs7::attribute_map<vector_range_type>& authenticated_attributes,
	const vector_range_type& authenticode_encrypted_digest,
	const x509::x509_certificate_store<x509::x509_certificate_ref<vector_range_type>>& cert_store,
	timestamp_signature_check_status<vector_range_type>& result);

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
