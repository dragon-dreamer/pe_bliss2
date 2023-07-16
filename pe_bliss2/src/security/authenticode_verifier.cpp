#include "pe_bliss2/security/authenticode_verifier.h"

#include <optional>
#include <string>
#include <system_error>

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/authenticode_loader.h"
#include "pe_bliss2/security/authenticode_page_hashes.h"

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
		case absent_certificates:
			return "No certificates are present in the Authenticode signature";
		case duplicate_certificates:
			return "Duplicate certificates are present in the Authenticode signature";
		case absent_signing_cert:
			return "Signing certificate is absent";
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

image_hash_verification_result verify_image_hash(span_range_type image_hash,
	digest_algorithm digest_alg, const image::image& instance,
	const std::optional<span_range_type>& page_hashes,
	const std::optional<page_hash_options>& page_hash_options)
{
	image_hash_verification_result result;

	image_hash_result hash_result;
	if (page_hashes && page_hash_options)
	{
		hash_result = calculate_hash(digest_alg, instance, &*page_hash_options);
		if (hash_result.page_hash_errc)
			result.page_hashes_check_errc = hash_result.page_hash_errc;
		result.page_hashes_valid = std::ranges::equal(hash_result.page_hashes, *page_hashes);
	}
	else
	{
		hash_result = calculate_hash(digest_alg, instance);
	}

	result.image_hash_valid = std::ranges::equal(hash_result.image_hash, image_hash);
	return result;
}

template<typename RangeType>
bool verify_message_digest(const authenticode_pkcs7<RangeType>& authenticode,
	const pkcs7::signer_info_ref<RangeType>& signer,
	const pkcs7::attribute_map<RangeType>& authenticated_attributes)
{
	const auto message_digest = pkcs7::calculate_message_digest(authenticode, signer);
	return pkcs7::verify_message_digest_attribute(message_digest,
		authenticated_attributes);
}

template<typename RangeType>
x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>> build_certificate_store(
	const authenticode_pkcs7<RangeType>& authenticode,
	error_list* warnings)
{
	x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>> store;

	const auto& content_info_data = authenticode.get_content_info().data;
	const auto& certificates = content_info_data.certificates;
	if (!certificates || certificates->empty())
	{
		if (warnings)
		{
			warnings->add_error(
				authenticode_verifier_errc::absent_certificates);
		}
		return store;
	}

	store.reserve(certificates->size());
	for (const auto& cert : *certificates)
	{
		std::visit([&store, warnings](const auto& contained_cert) {
			if (!store.add_certificate(contained_cert))
			{
				if (warnings)
				{
					warnings->add_error(
						authenticode_verifier_errc::duplicate_certificates);
				}
			}
		}, cert);
	}

	return store;
}

template<typename RangeType>
void verify_valid_format_authenticode(
	const authenticode_pkcs7<RangeType>& authenticode,
	const pkcs7::signer_info_ref<RangeType>& signer,
	const pkcs7::attribute_map<RangeType>& authenticated_attributes,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base& result)
{
	const auto digest_alg = signer.get_digest_algorithm();
	const auto digest_encryption_alg = signer.get_digest_encryption_algorithm();
	result.image_digest_alg = digest_alg;
	result.digest_encryption_alg = digest_encryption_alg;
	if (digest_alg == digest_algorithm::unknown)
	{
		result.authenticode_format_errors.add_error(
			authenticode_verifier_errc::unsupported_digest_algorithm);
		return;
	}

	if (digest_encryption_alg == digest_encryption_algorithm::unknown)
	{
		result.authenticode_format_errors.add_error(
			authenticode_verifier_errc::unsupported_digest_encryption_algorithm);
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

	const auto& content_info_data = authenticode.get_content_info().data;
	const auto& certificates = content_info_data.certificates;
	if (!certificates || certificates->empty())
	{
		result.authenticode_format_errors.add_error(
			authenticode_verifier_errc::absent_certificates);
		return;
	}

	const auto cert_store = build_certificate_store(
		authenticode, &result.certificate_store_warnings);

	const auto* signing_cert = cert_store.find_certificate(
		signer.get_signer_certificate_serial_number(),
		signer.get_signer_certificate_raw_issuer());
	if (!signing_cert)
	{
		result.authenticode_format_errors.add_error(
			authenticode_verifier_errc::absent_signing_cert);
		return;
	}

	try
	{
		result.signature_valid = pkcs7::verify_signature(
			signing_cert->get_public_key(),
			signer.calculate_authenticated_attributes_digest(),
			signer.get_encrypted_digest(),
			digest_alg, digest_encryption_alg);
	}
	catch (const std::exception&)
	{
		result.authenticode_processing_error = std::current_exception();
		return;
	}
}

template<typename RangeType>
void verify_authenticode(const authenticode_pkcs7<RangeType>& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base& result)
{
	validate(authenticode, result.authenticode_format_errors);
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

	validate(authenticated_attributes, result.authenticode_format_errors);
	if (result.authenticode_format_errors.has_errors())
		return;

	verify_valid_format_authenticode(
		authenticode, signer, authenticated_attributes, instance, opts, result);
}

template<typename RangeType>
authenticode_check_status verify_authenticode_full(const authenticode_pkcs7<RangeType>& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts)
{
	authenticode_check_status result;
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

	const auto nested_signatures = load_nested_signatures(unauthenticated_attributes);
	result.nested.reserve(nested_signatures.size());
	for (const auto& nested_signature : nested_signatures)
		verify_authenticode(nested_signature, instance, opts, result.nested.emplace_back());

	return result;
}

template void verify_valid_format_authenticode<span_range_type>(
	const authenticode_pkcs7<span_range_type>& authenticode,
	const pkcs7::signer_info_ref<span_range_type>& signer,
	const pkcs7::attribute_map<span_range_type>& authenticated_attributes,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base& result);
template void verify_valid_format_authenticode<vector_range_type>(
	const authenticode_pkcs7<vector_range_type>& authenticode,
	const pkcs7::signer_info_ref<vector_range_type>& signer,
	const pkcs7::attribute_map<vector_range_type>& authenticated_attributes,
	const image::image& instance,
	const authenticode_verification_options& opts,
	authenticode_check_status_base& result);

template bool verify_message_digest<span_range_type>(
	const authenticode_pkcs7<span_range_type>& authenticode,
	const pkcs7::signer_info_ref<span_range_type>& signer,
	const pkcs7::attribute_map<span_range_type>& authenticated_attributes);
template bool verify_message_digest<vector_range_type>(
	const authenticode_pkcs7<vector_range_type>& authenticode,
	const pkcs7::signer_info_ref<vector_range_type>& signer,
	const pkcs7::attribute_map<vector_range_type>& authenticated_attributes);

template x509::x509_certificate_store<x509::x509_certificate_ref<span_range_type>>
build_certificate_store<span_range_type>(
	const authenticode_pkcs7<span_range_type>& authenticode,
	error_list* errors);
template x509::x509_certificate_store<x509::x509_certificate_ref<vector_range_type>>
build_certificate_store<vector_range_type>(
	const authenticode_pkcs7<vector_range_type>& authenticode,
	error_list* errors);

template authenticode_check_status verify_authenticode_full<span_range_type>(
	const authenticode_pkcs7<span_range_type>& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts);
template authenticode_check_status verify_authenticode_full<vector_range_type>(
	const authenticode_pkcs7<vector_range_type>& authenticode,
	const image::image& instance,
	const authenticode_verification_options& opts);

} //namespace pe_bliss::security
