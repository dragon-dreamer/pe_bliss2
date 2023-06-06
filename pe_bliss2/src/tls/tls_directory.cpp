#include "pe_bliss2/tls/tls_directory.h"

#include <string>
#include <system_error>

namespace
{

struct tls_directory_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "tls_directory";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::tls::tls_directory_errc;
		switch (static_cast<pe_bliss::tls::tls_directory_errc>(ev))
		{
		case too_big_alignment_value:
			return "Too big TLS type alignment value";
		case invalid_alignment_value:
			return "Invalid TLS type alignment value";
		default:
			return {};
		}
	}
};

const tls_directory_error_category tls_directory_error_category_instance;

} //namespace

namespace pe_bliss::tls
{
std::error_code make_error_code(tls_directory_errc e) noexcept
{
	return { static_cast<int>(e), tls_directory_error_category_instance };
}
} //namespace pe_bliss::tls
