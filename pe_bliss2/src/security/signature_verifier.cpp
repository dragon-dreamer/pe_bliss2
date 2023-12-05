#include "pe_bliss2/security/signature_verifier.h"

#include <exception>
#include <string>

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
} //namespace

std::error_code make_error_code(signature_verifier_errc e) noexcept
{
	return { static_cast<int>(e), signature_verifier_error_category_instance };
}

} //namespace pe_bliss::security
