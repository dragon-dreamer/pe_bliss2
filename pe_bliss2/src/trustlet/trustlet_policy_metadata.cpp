#include "pe_bliss2/trustlet/trustlet_policy_metadata.h"

#include <string>
#include <system_error>

#include "pe_bliss2/pe_error.h"

namespace
{

struct trustlet_policy_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "trustlet_policy";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::trustlet::trustlet_policy_errc;
		switch (static_cast<pe_bliss::trustlet::trustlet_policy_errc>(ev))
		{
		case unsupported_version:
			return "Unsupported trustlet policy version";
		case invalid_string_address:
			return "Invalid trustlet policy entry string virtual address";
		case unable_to_read_value:
			return "Unable to read trustlet policy entry value";
		case unsupported_entry_type:
			return "Unsupported trustlet policy type";
		default:
			return {};
		}
	}
};

const trustlet_policy_error_category trustlet_policy_error_category_instance;

} //namespace

namespace pe_bliss::trustlet
{

std::error_code make_error_code(trustlet_policy_errc e) noexcept
{
	return { static_cast<int>(e), trustlet_policy_error_category_instance };
}

} //namespace pe_bliss::trustlet
