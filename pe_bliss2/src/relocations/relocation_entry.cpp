#include "pe_bliss2/relocations/relocation_entry.h"

#include <cassert>
#include <string>
#include <system_error>

#include "pe_bliss2/pe_error.h"

namespace
{

struct relocation_entry_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "relocation_entry";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::relocations::relocation_entry_errc;
		switch (static_cast<pe_bliss::relocations::relocation_entry_errc>(ev))
		{
		case relocation_param_is_absent:
			return "Relocation parameter is required for this type of relocation, but absent";
		case unsupported_relocation_type:
			return "Unsupported relocation type";
		case too_large_relocation_address:
			return "Too large relocation address";
		case invalid_relocation_entry:
			return "Invalid relocation entry";
		default:
			return {};
		}
	}
};

const relocation_entry_error_category relocation_entry_error_category_instance;

} //namespace

namespace pe_bliss::relocations
{

std::error_code make_error_code(relocation_entry_errc e) noexcept
{
	return { static_cast<int>(e), relocation_entry_error_category_instance };
}

relocation_type relocation_entry::get_type() const noexcept
{
	return static_cast<relocation_type>((descriptor_.get() & 0xf000u) >> 12u);
}

void relocation_entry::set_type(relocation_type type) noexcept
{
	descriptor_.get() &= ~0xf000u;
	descriptor_.get() |= static_cast<std::uint8_t>(type) << 12u;
}

relocation_entry::address_type relocation_entry::get_address() const noexcept
{
	return static_cast<address_type>(descriptor_.get() & 0xfffu);
}

void relocation_entry::set_address(address_type address)
{
	static constexpr std::uint16_t max_address = 0xfffu;
	if (address > max_address)
		throw pe_error(relocation_entry_errc::too_large_relocation_address);

	descriptor_.get() &= ~0xfffu;
	descriptor_.get() |= address;
}

std::uint64_t relocation_entry::apply_to(std::uint64_t value,
	std::uint64_t image_base_difference,
	core::file_header::machine_type machine) const
{
	using enum relocation_type;
	switch (get_type())
	{
	case absolute:
		return value;

	case highlow:
		assert(value <= (std::numeric_limits<std::uint32_t>::max)());
		return static_cast<std::uint32_t>(value + image_base_difference);

	case dir64:
		return value + image_base_difference;

	case high:
		assert(value <= (std::numeric_limits<std::uint16_t>::max)());
		return static_cast<std::uint16_t>(value + (image_base_difference >> 16u));

	case low:
		assert(value <= (std::numeric_limits<std::uint16_t>::max)());
		return static_cast<std::uint16_t>(value + image_base_difference);

	case highadj:
		{
			if (!param_)
				throw pe_error(relocation_entry_errc::relocation_param_is_absent);

			assert(value <= (std::numeric_limits<std::uint16_t>::max)());
			std::uint32_t result = static_cast<std::uint32_t>(value) << 16u;
			result += param_->get();
			result += static_cast<std::uint32_t>(image_base_difference);
			result += 0x8000u;
			return static_cast<std::uint16_t>(result >> 16u);
		}

	case thumb_mov32:
		if (machine != core::file_header::machine_type::armnt)
			break;

		{
			auto inst0 = static_cast<std::uint32_t>(value & 0xffffffffu);
			auto inst1 = static_cast<std::uint32_t>((value >> 32u) & 0xffffffffu);

			std::uint16_t lo = ((inst0 << 1u) & 0x0800u) + ((inst0 << 12u) & 0xf000u) +
				((inst0 >> 20u) & 0x0700u) + ((inst0 >> 16u) & 0x00ffu);
			std::uint16_t hi = ((inst1 << 1u) & 0x0800u) + ((inst1 << 12u) & 0xf000u) +
				((inst1 >> 20u) & 0x0700u) + ((inst1 >> 16u) & 0x00ffu);

			std::uint32_t adjusted = (static_cast<std::uint32_t>(lo) | (hi << 16u))
				+ static_cast<std::uint32_t>(image_base_difference);
			lo = static_cast<std::uint16_t>(adjusted & 0xffffu);
			hi = static_cast<std::uint16_t>((adjusted >> 16u) & 0xffffu);

			inst0 = (inst0 & 0x8f00fbf0u) + ((lo >> 1u) & 0x0400u) + ((lo >> 12u) & 0x000fu) +
				((lo << 20u) & 0x70000000u) + ((lo << 16u) & 0xff0000u);
			inst1 = (inst1 & 0x8f00fbf0u) + ((hi >> 1u) & 0x0400u) + ((hi >> 12u) & 0x000fu) +
				((hi << 20u) & 0x70000000u) + ((hi << 16u) & 0xff0000u);

			return static_cast<std::uint64_t>(inst0) | static_cast<std::uint64_t>(inst1) << 32ull;
		}
		break;

	case mips_jmpaddr:
	case riscv_low12s:
	case high3adj:
	case mips_jmpaddr16:
	default:
		break;
	}

	throw pe_error(relocation_entry_errc::unsupported_relocation_type);
}

std::uint8_t relocation_entry::get_affected_size_in_bytes(
	core::file_header::machine_type machine) const
{
	using enum relocation_type;
	switch (get_type())
	{
	case absolute:
		return 0u;

	case highlow:
		return sizeof(std::uint32_t);

	case dir64:
		return sizeof(std::uint64_t);

	case high:
	case low:
	case highadj:
		return sizeof(std::uint16_t);

	case thumb_mov32:
		if (machine == core::file_header::machine_type::armnt)
			return sizeof(std::uint64_t);
		break;

	case riscv_low12s:
	case high3adj:
	case mips_jmpaddr16:
	case mips_jmpaddr:
	default:
		break;
	}

	throw pe_error(relocation_entry_errc::unsupported_relocation_type);
}

} //namespace pe_bliss::relocations
