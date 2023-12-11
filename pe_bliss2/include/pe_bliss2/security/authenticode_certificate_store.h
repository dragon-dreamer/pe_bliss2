#pragma once

#include <optional>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/authenticode_pkcs7.h"
#include "pe_bliss2/security/authenticode_timestamp_signature.h"
#include "pe_bliss2/security/x509/x509_certificate.h"
#include "pe_bliss2/security/x509/x509_certificate_store.h"

namespace pe_bliss::security
{

enum class certificate_store_errc
{
	absent_certificates = 1,
	duplicate_certificates
};

std::error_code make_error_code(certificate_store_errc) noexcept;

template<typename RangeType>
[[nodiscard]]
x509::x509_certificate_store<x509::x509_certificate<RangeType>> build_certificate_store(
	const authenticode_pkcs7<RangeType>& authenticode,
	error_list* errors);

template<typename RangeType>
[[nodiscard]]
x509::x509_certificate_store<x509::x509_certificate<RangeType>> build_certificate_store(
	const authenticode_signature_cms_info_ms_bug_workaround_type<RangeType>& signature,
	error_list* warnings);

template<typename RangeType>
[[nodiscard]]
x509::x509_certificate_store<x509::x509_certificate<RangeType>> build_certificate_store(
	const authenticode_signature_cms_info_type<RangeType>& signature,
	error_list* warnings);

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::certificate_store_errc> : true_type {};
} //namespace std
