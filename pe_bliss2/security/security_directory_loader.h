#pragma once

#include <cstdint>
#include <optional>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/security/security_directory.h"

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::security
{

enum class security_directory_loader_errc
{
	invalid_directory = 1,
	invalid_entry,
	invalid_certificate_data,
	invalid_entry_size,
	invalid_directory_size,
	unaligned_directory,
	too_many_entries
};

std::error_code make_error_code(security_directory_loader_errc) noexcept;

struct [[nodiscard]] loader_options
{
	bool copy_raw_data = false;
	std::uint32_t max_entries = 10u;
};

[[nodiscard]]
std::optional<security_directory_details> load(const image::image& instance,
	const loader_options& options = {});

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::security_directory_loader_errc> : true_type {};
} //namespace std
