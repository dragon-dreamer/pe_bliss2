#include "pe_bliss2/relocations/relocation_entry.h"

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

void relocation_entry::set_address(address_type address) noexcept
{
	descriptor_.get() &= ~0xfffu;
	descriptor_.get() |= address;
}

std::uint64_t relocation_entry::apply_to(std::uint64_t value,
	std::uint64_t real_base_image_base_diff) const
{
	using enum relocation_type;
	switch (get_type())
	{
	case absolute:
		return value;

	case highlow:
		return static_cast<std::uint32_t>(value + real_base_image_base_diff);

	case dir64:
		return value + real_base_image_base_diff;

	case high:
		return static_cast<std::uint16_t>(((value >> 16u) + (real_base_image_base_diff >> 16u)) << 16u);

	case low:
		return static_cast<std::uint16_t>(value + real_base_image_base_diff);

	case highadj:
	{
		if (!param_)
			throw pe_error(relocation_entry_errc::relocation_param_is_absent);

		std::uint32_t result = static_cast<std::uint16_t>(value << 16u);
		result += param_->get();
		result += static_cast<std::uint32_t>(real_base_image_base_diff);
		result += 0x8000u;
		return static_cast<std::uint16_t>(result >> 16u);
	}

	case mips_jmpaddr:
	case thumb_mov32:
	case riscv_low12s:
	case high3adj:
	case mips_jmpaddr16:
	default:
		break;
	}

	throw pe_error(relocation_entry_errc::unsupported_relocation_type);
}

std::uint8_t relocation_entry::get_affected_size_in_bytes() const
{
	using enum relocation_type;
	switch (get_type())
	{
	case absolute:
		return 0;

	case highlow:
	case highadj:
		return sizeof(std::uint32_t);

	case dir64:
		return sizeof(std::uint64_t);

	case high:
	case low:
		return sizeof(std::uint16_t);

	case thumb_mov32:
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
