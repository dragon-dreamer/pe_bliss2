#pragma once

#include <exception>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/pkcs7/pkcs7.h"
#include "pe_bliss2/security/pkcs7/pkcs7_signature.h"
#include "pe_bliss2/security/pkcs7/signer_info.h"
#include "pe_bliss2/security/x509/x509_certificate.h"
#include "pe_bliss2/security/x509/x509_certificate_store.h"

namespace pe_bliss::security
{

enum class signature_verifier_errc
{
	absent_signing_cert = 1,
	absent_signing_cert_issuer_and_sn,
	unable_to_verify_signature
};

std::error_code make_error_code(signature_verifier_errc) noexcept;

struct [[nodiscard]] signature_verification_result
{
	pkcs7::signature_verification_result pkcs7_result;
	error_list errors;
	// Will contain exception if `errors`
	// contain `unable_to_verify_signature`.
	std::exception_ptr processing_error;

	[[nodiscard]]
	operator bool() const noexcept
	{
		return pkcs7_result.valid
			&& !errors.has_errors();
	}
};

namespace impl
{
template<typename Signer, typename RangeType>
signature_verification_result verify_signature_impl(
	const Signer& signer,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>>& cert_store)
{
	signature_verification_result result;

	const auto issuer_and_sn = signer.get_signer_certificate_issuer_and_serial_number();
	if (!issuer_and_sn.serial_number || !issuer_and_sn.issuer
		|| (issuer_and_sn.serial_number->empty() && issuer_and_sn.issuer->empty()))
	{
		result.errors.add_error(signature_verifier_errc::absent_signing_cert_issuer_and_sn);
		return result;
	}

	const auto* signing_cert = cert_store.find_certificate(
		*issuer_and_sn.serial_number,
		*issuer_and_sn.issuer);
	if (!signing_cert)
	{
		result.errors.add_error(signature_verifier_errc::absent_signing_cert);
		return result;
	}

	try
	{
		span_range_type signature_algorithm_parameters;
		if (const auto& params = signing_cert->get_signature_algorithm_parameters(); params)
			signature_algorithm_parameters = *params;

		result.pkcs7_result = pkcs7::verify_signature(
			signing_cert->get_public_key(),
			signer.calculate_authenticated_attributes_digest(),
			signer.get_encrypted_digest(),
			signer.get_digest_algorithm(),
			signer.get_digest_encryption_algorithm().encryption_alg,
			signature_algorithm_parameters);
	}
	catch (const std::exception&)
	{
		result.errors.add_error(signature_verifier_errc::unable_to_verify_signature);
		result.processing_error = std::current_exception();
	}
	return result;
}
} //namespace impl

template<typename RangeType1, typename RangeType2>
signature_verification_result verify_signature(
	const pkcs7::signer_info_ref_pkcs7<RangeType1>& signer,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType2>>& cert_store)
{
	return impl::verify_signature_impl(signer, cert_store);
}

template<typename RangeType1, typename RangeType2>
signature_verification_result verify_signature(
	const pkcs7::signer_info_ref_cms<RangeType1>& signer,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType2>>& cert_store)
{
	return impl::verify_signature_impl(signer, cert_store);
}

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::signature_verifier_errc> : true_type {};
} //namespace std
