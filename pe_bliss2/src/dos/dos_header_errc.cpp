#include "pe_bliss2/dos/dos_header_errc.h"

#include <system_error>

namespace
{

struct dos_header_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "dos_header";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::dos::dos_header_errc;
		switch (static_cast<pe_bliss::dos::dos_header_errc>(ev))
		{
		case invalid_dos_header_signature:
			return "Invalid DOS header signature";
		case unaligned_e_lfanew:
			return "PE header signature is not DWORD-aligned";
		case invalid_e_lfanew:
			return "Too big or too small e_lfanew value";
		case unable_to_read_dos_header:
			return "Unable to read DOS header";
		default:
			return {};
		}
	}
};

const dos_header_error_category dos_header_error_category_instance;

} //namespace

namespace pe_bliss::dos
{
std::error_code make_error_code(dos_header_errc e) noexcept
{
	return { static_cast<int>(e), dos_header_error_category_instance };
}
} //namespace pe_bliss::dos
