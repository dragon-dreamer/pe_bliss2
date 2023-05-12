#include "pe_bliss2/resources/manifest_accessor_interface.h"

#include <string>
#include <system_error>

namespace
{
struct manifest_loader_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "manifest_loader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::resources::manifest_loader_errc;
		switch (static_cast<pe_bliss::resources::manifest_loader_errc>(ev))
		{
		case invalid_xml:
			return "Invalid manifest XML";
		case absent_declaration:
			return "Absent manifest XML declaration";
		default:
			return {};
		}
	}
};

const manifest_loader_category manifest_loader_category_instance;
} //namespace

namespace pe_bliss::resources
{

std::error_code make_error_code(manifest_loader_errc e) noexcept
{
	return { static_cast<int>(e), manifest_loader_category_instance };
}

} //namespace pe_bliss::resources
