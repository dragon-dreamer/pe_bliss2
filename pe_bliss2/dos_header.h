#pragma once

#include <array>
#include <cstdint>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/detail/image_dos_header.h"
#include "pe_bliss2/detail/packed_struct_base.h"
#include "pe_bliss2/pe_error.h"

namespace buffers
{
class input_buffer_interface;
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss
{

enum class dos_header_errc
{
	invalid_dos_header_signature = 1,
	unaligned_e_lfanew,
	invalid_e_lfanew,
	unable_to_read_dos_header
};

struct dos_header_validation_options
{
	bool validate_e_lfanew = true;
	bool validate_magic = true;
};

std::error_code make_error_code(dos_header_errc) noexcept;

class dos_header : public detail::packed_struct_base<detail::image_dos_header>
{
public:
	///DOS header magic value ('MZ')
	static constexpr std::uint16_t mz_magic_value = 0x5a4d;
	///Minimum valid e_lfanew value
	static constexpr std::uint32_t min_e_lfanew = 4;
	///Maximum valid e_lfanew value (safety limit)
	static constexpr std::uint32_t max_e_lfanew = 10485760; //10 Mb

public:
	//When deserializing, buffer should point to the start of the image
	void deserialize(buffers::input_buffer_interface& buf,
		bool allow_virtual_memory = false);
	void serialize(buffers::output_buffer_interface& buf,
		bool write_virtual_part = true) const;

public:
	[[nodiscard]]
	pe_error_wrapper validate(
		const dos_header_validation_options& options) const noexcept;
	[[nodiscard]]
	pe_error_wrapper validate_magic() const noexcept;
	[[nodiscard]]
	pe_error_wrapper validate_e_lfanew() const noexcept;
};

} //namespace pe_bliss

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::dos_header_errc> : true_type {};
} //namespace std
