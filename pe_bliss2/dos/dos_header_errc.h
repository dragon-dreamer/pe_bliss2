#pragma once

#include <system_error>
#include <type_traits>

namespace pe_bliss::dos
{

enum class dos_header_errc
{
	invalid_dos_header_signature = 1,
	unaligned_e_lfanew,
	invalid_e_lfanew,
	unable_to_read_dos_header
};

std::error_code make_error_code(dos_header_errc) noexcept;

} //namespace pe_bliss::dos

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::dos::dos_header_errc> : true_type {};
} //namespace std
