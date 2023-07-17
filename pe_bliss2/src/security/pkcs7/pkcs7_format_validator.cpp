#include "pe_bliss2/security/pkcs7/pkcs7_format_validator.h"

#include <string>
#include <system_error>

namespace
{

struct pkcs7_format_validator_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "pkcs7_format_validator";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::pkcs7::pkcs7_format_validator_errc;
		switch (static_cast<pe_bliss::security::pkcs7::pkcs7_format_validator_errc>(ev))
		{
		case invalid_signed_data_oid:
			return "Invalid signed data OID";
		case invalid_signed_data_version:
			return "Invalid signed data version (must be 1)";
		case invalid_signer_count:
			return "Invalid signer count (must be 1)";
		case non_matching_digest_algorithm:
			return "Digest algorithm of signed data does not match digest algorithm of signer info";
		case invalid_signer_info_version:
			return "Invalid signer info version (must be 1)";
		case absent_message_digest:
			return "Absent message digest attribute";
		case invalid_message_digest:
			return "Invalid message digest attribute";
		case absent_content_type:
			return "Absent content type attribute";
		case invalid_content_type:
			return "Invalid content type attribute value";
		case invalid_signing_time:
			return "Invalid signing time";
		default:
			return {};
		}
	}
};

const pkcs7_format_validator_error_category pkcs7_format_validator_error_category_instance;

} //namespace

namespace pe_bliss::security::pkcs7
{

std::error_code make_error_code(pkcs7_format_validator_errc e) noexcept
{
	return { static_cast<int>(e), pkcs7_format_validator_error_category_instance };
}

} //namespace pe_bliss::security::pkcs7
