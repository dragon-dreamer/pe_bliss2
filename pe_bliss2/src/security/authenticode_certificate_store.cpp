#include "pe_bliss2/security/authenticode_certificate_store.h"

namespace pe_bliss::security
{

namespace
{

struct certificate_store_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "certificate_store";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::certificate_store_errc;
		switch (static_cast<pe_bliss::security::certificate_store_errc>(ev))
		{
		case absent_certificates:
			return "No certificates are present in the Authenticode signature";
		case duplicate_certificates:
			return "Duplicate certificates are present in the Authenticode signature";
		default:
			return {};
		}
	}
};

const certificate_store_error_category certificate_store_error_category_instance;

template<typename RangeType, typename Signature>
x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>> build_certificate_store_impl(
	const Signature& signature,
	error_list* warnings)
{
	x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>> store;

	const auto& content_info_data = signature.get_content_info().data;
	const auto& certificates = content_info_data.certificates;
	if (!certificates || certificates->empty())
	{
		if (warnings)
			warnings->add_error(certificate_store_errc::absent_certificates);

		return store;
	}

	store.reserve(certificates->size());
	for (const auto& cert : *certificates)
	{
		std::visit([&store, warnings](const auto& contained_cert) {
			if constexpr (std::is_same_v<decltype(contained_cert),
			const asn1::crypto::x509::certificate<RangeType>&>)
			{
				if (!store.add_certificate(contained_cert))
				{
					if (warnings)
					{
						warnings->add_error(
							certificate_store_errc::duplicate_certificates);
					}
				}
			}
		}, cert);
	}

	return store;
}
} //namespace

std::error_code make_error_code(certificate_store_errc e) noexcept
{
	return { static_cast<int>(e), certificate_store_error_category_instance };
}

template<typename RangeType>
x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>> build_certificate_store(
	const authenticode_pkcs7<RangeType>& authenticode,
	error_list* warnings)
{
	return build_certificate_store_impl<RangeType>(authenticode, warnings);
}

template<typename RangeType>
x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>> build_certificate_store(
	const authenticode_signature_cms_info_ms_bug_workaround_type<RangeType>& signature,
	error_list* warnings)
{
	return build_certificate_store_impl<RangeType>(signature, warnings);
}

template<typename RangeType>
x509::x509_certificate_store<x509::x509_certificate_ref<RangeType>> build_certificate_store(
	const authenticode_signature_cms_info_type<RangeType>& signature,
	error_list* warnings)
{
	return build_certificate_store_impl<RangeType>(signature, warnings);
}

template x509::x509_certificate_store<x509::x509_certificate_ref<span_range_type>>
build_certificate_store<span_range_type>(
	const authenticode_pkcs7<span_range_type>& authenticode,
	error_list* errors);
template x509::x509_certificate_store<x509::x509_certificate_ref<vector_range_type>>
build_certificate_store<vector_range_type>(
	const authenticode_pkcs7<vector_range_type>& authenticode,
	error_list* errors);

template x509::x509_certificate_store<x509::x509_certificate_ref<span_range_type>>
build_certificate_store<span_range_type>(
	const authenticode_signature_cms_info_ms_bug_workaround_type<span_range_type>& signature,
	error_list* warnings);
template x509::x509_certificate_store<x509::x509_certificate_ref<vector_range_type>>
build_certificate_store<vector_range_type>(
	const authenticode_signature_cms_info_ms_bug_workaround_type<vector_range_type>& signature,
	error_list* warnings);

template x509::x509_certificate_store<x509::x509_certificate_ref<span_range_type>>
build_certificate_store<span_range_type>(
	const authenticode_signature_cms_info_type<span_range_type>& signature,
	error_list* warnings);
template x509::x509_certificate_store<x509::x509_certificate_ref<vector_range_type>>
build_certificate_store<vector_range_type>(
	const authenticode_signature_cms_info_type<vector_range_type>& signature,
	error_list* warnings);

} //namespace pe_bliss::security
