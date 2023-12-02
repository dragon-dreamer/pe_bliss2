#include "pe_bliss2/security/signature_verifier.h"

#include <optional>

#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/pkcs7/pkcs7_signature.h"

namespace pe_bliss::security
{

namespace
{

struct signature_verifier_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "signature_verifier";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::signature_verifier_errc;
		switch (static_cast<pe_bliss::security::signature_verifier_errc>(ev))
		{
		case absent_signing_cert:
			return "Signing certificate is absent";
		case absent_signing_cert_issuer_and_sn:
			return "Signing certificate issuer and serial number are absent";
		case unable_to_verify_signature:
			return "Unable to verify signature";
		default:
			return {};
		}
	}
};

const signature_verifier_error_category signature_verifier_error_category_instance;

template<typename Signer, typename RangeType>
pkcs7::signature_verification_result verify_signature_impl(
	const Signer& signer,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>>& cert_store,
	error_list& errors,
	std::exception_ptr& processing_error)
{
	auto issuer_and_sn = signer.get_signer_certificate_issuer_and_serial_number();
	if (!issuer_and_sn.serial_number || !issuer_and_sn.issuer)
	{
		errors.add_error(signature_verifier_errc::absent_signing_cert_issuer_and_sn);
		return {};
	}

	const auto* signing_cert = cert_store.find_certificate(
		*issuer_and_sn.serial_number,
		*issuer_and_sn.issuer);
	if (!signing_cert)
	{
		errors.add_error(signature_verifier_errc::absent_signing_cert);
		return {};
	}

	try
	{
		std::optional<span_range_type> signature_algorithm_parameters;
		if (auto params = signing_cert->get_signature_algorithm_parameters(); params)
			signature_algorithm_parameters = *params;

		return pkcs7::verify_signature(
			signing_cert->get_public_key(),
			signer.calculate_authenticated_attributes_digest(),
			signer.get_encrypted_digest(),
			signer.get_digest_algorithm(),
			signer.get_digest_encryption_algorithm().encryption_alg,
			signature_algorithm_parameters ? *signature_algorithm_parameters : span_range_type{});
	}
	catch (const std::exception&)
	{
		errors.add_error(signature_verifier_errc::unable_to_verify_signature);
		processing_error = std::current_exception();
		return {};
	}
}
} //namespace

std::error_code make_error_code(signature_verifier_errc e) noexcept
{
	return { static_cast<int>(e), signature_verifier_error_category_instance };
}

template<typename RangeType>
pkcs7::signature_verification_result verify_signature(
	const pkcs7::signer_info_ref_pkcs7<RangeType>& signer,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>>& cert_store,
	error_list& errors,
	std::exception_ptr& processing_error)
{
	return verify_signature_impl(signer, cert_store, errors, processing_error);
}

template<typename RangeType>
pkcs7::signature_verification_result verify_signature(
	const pkcs7::signer_info_ref_cms<RangeType>& signer,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>>& cert_store,
	error_list& errors,
	std::exception_ptr& processing_error)
{
	return verify_signature_impl(signer, cert_store, errors, processing_error);
}

template pkcs7::signature_verification_result verify_signature<span_range_type>(
	const pkcs7::signer_info_ref_pkcs7<span_range_type>& signer,
	const x509::x509_certificate_store<x509::x509_certificate_ref<span_range_type>>& cert_store,
	error_list& errors,
	std::exception_ptr& processing_error);
template pkcs7::signature_verification_result verify_signature<vector_range_type>(
	const pkcs7::signer_info_ref_pkcs7<vector_range_type>& signer,
	const x509::x509_certificate_store<x509::x509_certificate_ref<vector_range_type>>& cert_store,
	error_list& errors,
	std::exception_ptr& processing_error);
template pkcs7::signature_verification_result verify_signature<span_range_type>(
	const pkcs7::signer_info_ref_cms<span_range_type>& signer,
	const x509::x509_certificate_store<x509::x509_certificate_ref<span_range_type>>& cert_store,
	error_list& errors,
	std::exception_ptr& processing_error);
template pkcs7::signature_verification_result verify_signature<vector_range_type>(
	const pkcs7::signer_info_ref_cms<vector_range_type>& signer,
	const x509::x509_certificate_store<x509::x509_certificate_ref<vector_range_type>>& cert_store,
	error_list& errors,
	std::exception_ptr& processing_error);

} //namespace pe_bliss::security
