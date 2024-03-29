#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/detail/packed_struct_base.h"
#include "pe_bliss2/detail/relocations/image_base_relocation.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/packed_struct.h"

namespace pe_bliss::relocations
{

enum class relocation_entry_errc
{
	relocation_param_is_absent = 1,
	unsupported_relocation_type,
	too_large_relocation_address,
	invalid_relocation_entry
};

std::error_code make_error_code(relocation_entry_errc) noexcept;

enum class relocation_type : std::uint8_t
{
	absolute = 0,
	high = 1,
	low = 2,
	highlow = 3,
	highadj = 4,
	mips_jmpaddr = 5,
	arm_mov32 = 5,
	riscv_high20 = 5,
	thumb_mov32 = 7,
	riscv_low12i = 7,
	riscv_low12s = 8,
	mips_jmpaddr16 = 9,
	dir64 = 10,
	high3adj = 11
};

class [[nodiscard]] relocation_entry
	: public detail::packed_struct_base<detail::relocations::type_or_offset_entry>
{
public:
	using address_type = std::uint16_t;
	using optional_param_type = std::optional<
		packed_struct<std::uint16_t>>;

public:
	[[nodiscard]]
	const optional_param_type& get_param() const noexcept
	{
		return param_;
	}

	[[nodiscard]]
	optional_param_type& get_param() noexcept
	{
		return param_;
	}

	[[nodiscard]]
	relocation_type get_type() const noexcept;

	void set_type(relocation_type type) noexcept;

	[[nodiscard]]
	address_type get_address() const noexcept;

	void set_address(address_type address);

	// image_base_difference is
	// (real image base) minus (image base from image headers)
	[[nodiscard("Discarding relocated value")]]
	std::uint64_t apply_to(std::uint64_t value,
		std::uint64_t image_base_difference,
		core::file_header::machine_type machine) const;

	[[nodiscard]]
	std::uint8_t get_affected_size_in_bytes(
		core::file_header::machine_type machine) const;

	[[nodiscard]]
	bool requires_parameter() const noexcept
	{
		return get_type() == relocation_type::highadj;
	}

private:
	optional_param_type param_;
};

class [[nodiscard]] relocation_entry_details
	: public relocation_entry
	, public error_list
{
};

} //namespace pe_bliss::relocations

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::relocations::relocation_entry_errc> : true_type {};
} //namespace std
