#pragma once

#include <exception>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/pkcs7/pkcs7.h"
#include "pe_bliss2/security/pkcs7/pkcs7_signature.h"
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

template<typename RangeType>
pkcs7::signature_verification_result verify_signature(
	const pkcs7::signer_info_ref_pkcs7<RangeType>& signer,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>>& cert_store,
	error_list& errors,
	std::exception_ptr& processing_error);
template<typename RangeType>
pkcs7::signature_verification_result verify_signature(
	const pkcs7::signer_info_ref_cms<RangeType>& signer,
	const x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>>& cert_store,
	error_list& errors,
	std::exception_ptr& processing_error);

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::signature_verifier_errc> : true_type {};
} //namespace std
