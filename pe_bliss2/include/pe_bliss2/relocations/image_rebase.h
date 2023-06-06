#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/relocations/base_relocation.h"

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::relocations
{

enum class rebase_errc
{
	unable_to_rebase_inexistent_data = 1,
	invalid_relocation_address
};

std::error_code make_error_code(rebase_errc) noexcept;

struct [[nodiscard]] rebase_options
{
	std::uint64_t new_base{};
	bool ignore_virtual_data = true;
};

void rebase(image::image& instance,
	const base_relocation_details_list& relocs,
	const rebase_options& options);
void rebase(image::image& instance,
	const base_relocation_list& relocs,
	const rebase_options& options);

} //namespace pe_bliss::relocations

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::relocations::rebase_errc> : true_type {};
} //namespace std
