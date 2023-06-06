#include "pe_bliss2/exceptions/arm/arm_exception_directory.h"

#include <system_error>

namespace
{

struct arm_exception_directory_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "arm_exception_directory";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::exceptions::arm::exception_directory_errc;
		switch (static_cast<pe_bliss::exceptions::arm::exception_directory_errc>(ev))
		{
		case invalid_function_length:
			return "Invalid function length";
		case invalid_non_volatile_register_count:
			return "Invalid non-volatile register count";
		case invalid_stack_adjust_value:
			return "Invalid stack adjust value";
		case invalid_stack_adjust_flags:
			return "Invalid stack adjust flags";
		case invalid_allocation_size:
			return "Invalid allocation size";
		case invalid_delta:
			return "Invalid delta";
		case invalid_registers:
			return "Invalid registers";
		default:
			return {};
		}
	}
};

const arm_exception_directory_error_category arm_exception_directory_error_category_instance;

} //namespace

namespace pe_bliss::exceptions::arm
{

std::error_code make_error_code(exception_directory_errc e) noexcept
{
	return { static_cast<int>(e), arm_exception_directory_error_category_instance };
}

packed_unwind_data::flag packed_unwind_data::get_flag() const noexcept
{
	return static_cast<flag>(unwind_data_ & 0b11u);
}

std::uint16_t packed_unwind_data::get_function_length() const noexcept
{
	return static_cast<std::uint16_t>(((unwind_data_ & 0x1ffcu) >> 2u) * 2u);
}

packed_unwind_data::ret packed_unwind_data::get_ret() const noexcept
{
	return static_cast<ret>((unwind_data_ & 0x6000u) >> 13u);
}

bool packed_unwind_data::homes_integer_parameter_registers() const noexcept
{
	return (unwind_data_ & 0x8000u) != 0u;
}

std::uint8_t packed_unwind_data::get_last_saved_non_volatile_register_index() const noexcept
{
	auto value = static_cast<std::uint8_t>((unwind_data_ & 0x70000u) >> 16u);
	switch (get_saved_non_volatile_registers())
	{
	case saved_non_volatile_registers::integer:
		return value + 4u;
	case saved_non_volatile_registers::floating_point:
		return value + 8u;
	default:
		return 0u;
	}
}

packed_unwind_data::saved_non_volatile_registers
	packed_unwind_data::get_saved_non_volatile_registers() const noexcept
{
	if (unwind_data_ & 0x80000u)
	{
		if ((unwind_data_ & 0x70000u) == 0x70000u)
			return saved_non_volatile_registers::none;
		return saved_non_volatile_registers::floating_point;
	}
	else
	{
		return saved_non_volatile_registers::integer;
	}
}

bool packed_unwind_data::save_restore_lr() const noexcept
{
	return (unwind_data_ & 0x100000u) != 0u;
}

bool packed_unwind_data::includes_extra_instructions() const noexcept
{
	return (unwind_data_ & 0x200000u) != 0u;
}

std::uint16_t packed_unwind_data::get_stack_adjust() const noexcept
{
	auto value = get_stack_adjust_raw();
	if (value >= 0x3f4u)
		return 0x3f0u * 4u;
	else
		return value * 4u;
}

std::optional<stack_adjust_flags> packed_unwind_data
	::get_stack_adjust_flags() const noexcept
{
	auto value = get_stack_adjust_raw();
	if (value < 0x3f4u)
		return {};

	stack_adjust_flags result{};
	result.stack_adjustment_words_number = (value & 0b0011u) + 1u;
	result.prologue_folding = (value & 0b0100u) != 0u;
	result.epilogue_folding = (value & 0b1000u) != 0u;
	return result;
}

void packed_unwind_data::set_flag(flag value) noexcept
{
	unwind_data_ &= ~0b11u;
	unwind_data_ |= static_cast<std::uint32_t>(value) & 0b11u;
}

void packed_unwind_data::set_function_length(std::uint16_t value)
{
	if (value % 2u)
		throw pe_error(exception_directory_errc::invalid_function_length);

	value /= 2u;
	if (value > 0x7ffu)
		throw pe_error(exception_directory_errc::invalid_function_length);

	unwind_data_ &= ~0x1ffcu;
	unwind_data_ |= (value << 2u);
}

void packed_unwind_data::set_ret(ret value) noexcept
{
	unwind_data_ &= ~0x6000u;
	unwind_data_ |= (static_cast<std::uint32_t>(value) & 0b11u) << 13u;
}

void packed_unwind_data::set_homes_integer_parameter_registers(bool value) noexcept
{
	if (value)
		unwind_data_ |= 0x8000u;
	else
		unwind_data_ &= ~0x8000u;
}

void packed_unwind_data::set_saved_non_volatile_registers(
	saved_non_volatile_registers value)
{
	if (get_saved_non_volatile_registers() == saved_non_volatile_registers::none)
		unwind_data_ = ~0x70000u;

	switch (value)
	{
	case saved_non_volatile_registers::integer:
		unwind_data_ &= ~0x80000u;
		break;
	case saved_non_volatile_registers::none:
		unwind_data_ |= 0x70000u | 0x80000u;
		break;
	default: //floating_point
		if ((unwind_data_ & 0x70000u) == 0x70000u)
			throw pe_error(exception_directory_errc::invalid_non_volatile_register_count);
		unwind_data_ |= 0x80000u;
		break;
	}
}

void packed_unwind_data::set_last_saved_non_volatile_register_index(std::uint8_t index)
{
	switch (get_saved_non_volatile_registers())
	{
	case saved_non_volatile_registers::integer:
		if (index < 4u || index > 11u)
			throw pe_error(exception_directory_errc::invalid_non_volatile_register_count);
		index -= 4u;
		break;
	case saved_non_volatile_registers::floating_point:
		if (index < 8u || index > 14u)
			throw pe_error(exception_directory_errc::invalid_non_volatile_register_count);
		index -= 8u;
		break;
	default:
		throw pe_error(exception_directory_errc::invalid_non_volatile_register_count);
	}

	unwind_data_ &= ~0x70000u;
	unwind_data_ |= (index << 16u);
}

void packed_unwind_data::set_save_restore_lr(bool value) noexcept
{
	if (value)
		unwind_data_ |= 0x100000u;
	else
		unwind_data_ &= ~0x100000u;
}

void packed_unwind_data::set_includes_extra_instructions(bool value) noexcept
{
	if (value)
		unwind_data_ |= 0x200000u;
	else
		unwind_data_ &= ~0x200000u;
}

void packed_unwind_data::set_stack_adjust(std::uint16_t value)
{
	if (value % 4u)
		throw pe_error(exception_directory_errc::invalid_stack_adjust_value);

	value /= 4u;
	if (value > 0x3f3u)
		throw pe_error(exception_directory_errc::invalid_stack_adjust_value);

	unwind_data_ &= ~0xffc00000u;
	unwind_data_ |= static_cast<std::uint32_t>(value) << 22u;
}

void packed_unwind_data::set_stack_adjust_flags(const stack_adjust_flags& flags)
{
	if (get_stack_adjust() != 0x3f0u * 4u)
		throw pe_error(exception_directory_errc::invalid_stack_adjust_value);

	if (flags.stack_adjustment_words_number < 1u || flags.stack_adjustment_words_number > 4u)
		throw pe_error(exception_directory_errc::invalid_stack_adjust_flags);
	if (!flags.prologue_folding && !flags.epilogue_folding)
		throw pe_error(exception_directory_errc::invalid_stack_adjust_flags);

	unwind_data_ &= ~0x3c00000u;
	if (flags.prologue_folding)
		unwind_data_ |= 0x1000000u;
	if (flags.epilogue_folding)
		unwind_data_ |= 0x2000000u;
	unwind_data_ |= static_cast<std::uint32_t>(flags.stack_adjustment_words_number - 1u) << 22u;
}

std::uint16_t packed_unwind_data::get_stack_adjust_raw() const noexcept
{
	return static_cast<std::uint16_t>((unwind_data_ & 0xffc00000u) >> 22u);
}

namespace opcode
{

std::uint16_t alloc_s::get_allocation_size() const noexcept
{
	return static_cast<std::uint16_t>(get_value<1, 7>()) * 4u;
}

void alloc_s::set_allocation_size(std::uint16_t size)
{
	set_scaled_value<4u, 1, 7, exception_directory_errc::invalid_allocation_size>(size);
}

int_registers::value save_r0r12_lr::get_saved_registers() const noexcept
{
	return static_cast<int_registers::value>(get_value<2, 15>());
}

void save_r0r12_lr::set_saved_registers(int_registers::value value)
{
	set_value<2, 15>(static_cast<std::uint16_t>(value));
}

std::uint8_t mov_sprx::get_delta() const noexcept
{
	return static_cast<std::uint8_t>(get_value<4, 7>());
}

void mov_sprx::set_delta(std::uint8_t delta)
{
	set_scaled_value<0u, 4, 7, exception_directory_errc::invalid_delta>(delta);
}

fp_registers::value save_d8dx::get_saved_registers() const noexcept
{
	std::uint32_t max_register = get_value<5, 7>() + 8u;
	auto register_mask = ((1u << (max_register + 1u)) - 1u) & 0xff00u;
	return static_cast<fp_registers::value>(register_mask);
}

void save_d8dx::set_saved_registers(fp_registers::value value)
{
	auto register_mask = static_cast<std::uint32_t>(value);
	if (register_mask & 0xffu)
		throw pe_error(exception_directory_errc::invalid_registers);
	if ((register_mask & 0x100u) != 0x100u)
		throw pe_error(exception_directory_errc::invalid_registers);
	if (!std::has_single_bit((register_mask >> 8u) + 1))
		throw pe_error(exception_directory_errc::invalid_registers);

	std::uint16_t opcode_value = 0u;
	auto max_register = std::bit_width(register_mask);
	if (max_register)
		opcode_value |= (max_register - 9u);

	set_value<5, 7>(opcode_value);
}

std::uint16_t alloc_s_wide::get_allocation_size() const noexcept
{
	return static_cast<std::uint16_t>(get_value<6, 15>()) * 4u;
}

void alloc_s_wide::set_allocation_size(std::uint16_t size)
{
	set_scaled_value<4u, 6, 15, exception_directory_errc::invalid_allocation_size>(size);
}

int_registers::value save_r0r7_lr::get_saved_registers() const noexcept
{
	std::uint16_t result = get_value<8, 15>();
	if (get_value<7, 7>())
		result |= static_cast<std::uint16_t>(int_registers::lr);

	return static_cast<int_registers::value>(result);
}

void save_r0r7_lr::set_saved_registers(int_registers::value value)
{
	std::uint16_t int_value = static_cast<std::uint16_t>(value);
	if ((value & ~static_cast<std::uint16_t>(int_registers::lr)) >= int_registers::r8)
		throw pe_error(exception_directory_errc::invalid_registers);
	if (int_value & static_cast<std::uint16_t>(int_registers::lr))
	{
		int_value &= ~static_cast<std::uint16_t>(int_registers::lr);
		int_value |= 0x100u;
	}
	set_scaled_value<0u, 7, 15, exception_directory_errc::invalid_registers>(int_value);
}

bool ms_specific::is_available_opcode() const noexcept
{
	return get_value<8, 11>() != 0u;
}

bool ldr_lr_sp::is_available_opcode() const noexcept
{
	return get_value<8, 11>() != 0u;
}

std::uint8_t ldr_lr_sp::get_delta() const noexcept
{
	return static_cast<std::uint8_t>(get_value<12, 15>()) * 4u;
}

void ldr_lr_sp::set_delta(std::uint8_t delta)
{
	set_scaled_value<4u, 12, 15, exception_directory_errc::invalid_delta>(delta);
}

} //namespace opcode

} //namespace pe_bliss::exceptions::arm
