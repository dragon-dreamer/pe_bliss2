#include "pe_bliss2/security/authenticode_check_status.h"

#include <string>
#include <system_error>

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

} //namespace pe_bliss::security
