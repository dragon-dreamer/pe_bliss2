#include "pe_bliss2/security/authenticode_format_validator_errc.h"

#include <string>
#include <system_error>

namespace
{

struct authenticode_format_validator_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "authenticode_format_validator";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::authenticode_format_validator_errc;
		switch (static_cast<pe_bliss::security::authenticode_format_validator_errc>(ev))
		{
		case non_matching_type_value_digest_algorithm:
			return "Digest algorithm of signer info does not match digest algorithm of DigestInfo";
		case invalid_content_info_oid:
			return "Invalid content info OID";
		case invalid_type_value_type:
			return "Invalid authenticode content OID";
		default:
			return {};
		}
	}
};

const authenticode_format_validator_error_category authenticode_format_validator_error_category_instance;

} //namespace

namespace pe_bliss::security
{

std::error_code make_error_code(authenticode_format_validator_errc e) noexcept
{
	return { static_cast<int>(e), authenticode_format_validator_error_category_instance };
}

} //namespace pe_bliss::security
