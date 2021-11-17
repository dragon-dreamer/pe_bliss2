#include "pe_bliss2/exceptions/arm64/arm64_exception_directory.h"

#include <array>
#include <system_error>

#include "pe_bliss2/pe_error.h"

namespace
{

struct arm64_exception_directory_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "arm64_exception_directory";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::exceptions::arm64::exception_directory_errc;
		switch (static_cast<pe_bliss::exceptions::arm64::exception_directory_errc>(ev))
		{
		case invalid_function_length:
			return "Invalid function length";
		case invalid_frame_size:
			return "Invalid frame size";
		case invalid_reg_int_value:
			return "Invalid reg INT value";
		case invalid_reg_fp_value:
			return "Invalid reg FP value";
		case unknown_unwind_code:
			return "Unknown unwind code";
		case invalid_allocation_size:
			return "Invalid allocation size";
		case invalid_offset:
			return "Invalid offset";
		case invalid_register:
			return "Invalid register";
		case invalid_custom_stack_case:
			return "Invalid custom stack case";
		case invalid_delta:
			return "Invalid delta";
		default:
			return {};
		}
	}
};

const arm64_exception_directory_error_category arm64_exception_directory_error_category_instance;

} //namespace

namespace pe_bliss::exceptions::arm64
{

std::error_code make_error_code(exception_directory_errc e) noexcept
{
	return { static_cast<int>(e), arm64_exception_directory_error_category_instance };
}

void packed_unwind_data::set_flag(flag value) noexcept
{
	unwind_data_ &= ~0b11u;
	unwind_data_ |= static_cast<std::uint8_t>(value) & 0b11u;
}

void packed_unwind_data::set_function_length(std::uint16_t length)
{
	if (length % 4u)
		throw pe_error(exception_directory_errc::invalid_function_length);

	length /= 4u;
	if (length > 0x7ffu)
		throw pe_error(exception_directory_errc::invalid_function_length);

	unwind_data_ &= ~0x1ffcu;
	unwind_data_ |= static_cast<std::uint32_t>(length) << 2u;
}

void packed_unwind_data::set_frame_size(std::uint16_t size)
{
	if (size % 16u)
		throw pe_error(exception_directory_errc::invalid_frame_size);

	size /= 16u;
	if (size > 0x1ffu)
		throw pe_error(exception_directory_errc::invalid_frame_size);

	unwind_data_ &= ~0xff800000u;
	unwind_data_ |= static_cast<std::uint32_t>(size) << 23u;
}

void packed_unwind_data::set_cr(cr value) noexcept
{
	unwind_data_ &= ~0x600000u;
	unwind_data_ |= (static_cast<std::uint32_t>(value) & 0b11u) << 21u;
}

void packed_unwind_data::set_homes_integer_parameter_registers(bool value) noexcept
{
	if (value)
		unwind_data_ |= 0x100000u;
	else
		unwind_data_ &= ~0x100000u;
}

void packed_unwind_data::set_reg_int(std::uint8_t value)
{
	if (value > 0xfu)
		throw pe_error(exception_directory_errc::invalid_reg_int_value);

	unwind_data_ &= ~0xf0000u;
	unwind_data_ |= static_cast<std::uint32_t>(value) << 16u;
}

void packed_unwind_data::set_reg_fp(std::uint8_t value)
{
	if (value > 0x7u || value == 1u)
		throw pe_error(exception_directory_errc::invalid_reg_fp_value);

	if (value)
		--value;

	unwind_data_ &= ~0xe000u;
	unwind_data_ |= static_cast<std::uint32_t>(value) << 13u;
}

struct unwind_code_match
{
	std::uint8_t mask;
	std::uint8_t match;
	unwind_code code;
};

unwind_code decode_unwind_code(std::byte value)
{
	using enum unwind_code;
	static constexpr std::array matches{
		unwind_code_match{ 0b11100000, 0b00000000, alloc_s },
		unwind_code_match{ 0b11100000, 0b00100000, save_r19r20_x },
		unwind_code_match{ 0b11000000, 0b01000000, save_fplr },
		unwind_code_match{ 0b11000000, 0b10000000, save_fplr_x },
		unwind_code_match{ 0b11111000, 0b11000000, alloc_m },
		unwind_code_match{ 0b11111100, 0b11001000, save_regp },
		unwind_code_match{ 0b11111100, 0b11001100, save_regp_x },
		unwind_code_match{ 0b11111100, 0b11010000, save_reg },
		unwind_code_match{ 0b11111110, 0b11010100, save_reg_x },
		unwind_code_match{ 0b11111110, 0b11010110, save_lrpair },
		unwind_code_match{ 0b11111110, 0b11011000, save_fregp },
		unwind_code_match{ 0b11111110, 0b11011010, save_fregp_x },
		unwind_code_match{ 0b11111110, 0b11011100, save_freg },
		unwind_code_match{ 0b11111111, 0b11011110, save_freg_x },
		unwind_code_match{ 0b11111111, 0b11100000, alloc_l },
		unwind_code_match{ 0b11111111, 0b11100001, set_fp },
		unwind_code_match{ 0b11111111, 0b11100010, add_fp },
		unwind_code_match{ 0b11111111, 0b11100011, nop },
		unwind_code_match{ 0b11111111, 0b11100100, end },
		unwind_code_match{ 0b11111111, 0b11100101, end_c },
		unwind_code_match{ 0b11111111, 0b11100110, save_next },
		unwind_code_match{ 0b11111111, 0b11100111, save_reg_any },
		unwind_code_match{ 0b11111000, 0b11101000, reserved_custom_stack },
		unwind_code_match{ 0b11110000, 0b11110000, reserved2 }
	};

	auto code_value = std::to_integer<std::uint8_t>(value);
	for (const auto& match : matches)
	{
		if ((match.mask & code_value) == match.match)
			return match.code;
	}

	throw pe_error(exception_directory_errc::unknown_unwind_code);
}

namespace opcode
{

std::uint16_t alloc_s::get_allocation_size() const noexcept
{
	return static_cast<std::uint16_t>(get_value<3, 7>()) * 16u;
}

void alloc_s::set_allocation_size(std::uint16_t size)
{
	set_scaled_value<16u, 3, 7, exception_directory_errc::invalid_allocation_size>(size);
}

std::uint16_t save_r19r20_x::get_offset() const noexcept
{
	return static_cast<std::uint16_t>(get_value<3, 7>()) * 8u;
}

void save_r19r20_x::set_offset(std::uint16_t offset)
{
	set_scaled_value<8u, 3, 7, exception_directory_errc::invalid_offset>(offset);
}

std::uint16_t save_fplr::get_offset() const noexcept
{
	return static_cast<std::uint16_t>(get_value<2, 7>()) * 8u;
}

void save_fplr::set_offset(std::uint16_t offset)
{
	set_scaled_value<8u, 2, 7, exception_directory_errc::invalid_offset>(offset);
}

std::uint16_t save_fplr_x::get_offset() const noexcept
{
	return static_cast<std::uint16_t>(get_value<2, 7>() + 1u) * 8u;
}

void save_fplr_x::set_offset(std::uint16_t offset)
{
	if (offset < 8u)
		throw pe_error(exception_directory_errc::invalid_offset);

	offset -= 8u;
	set_scaled_value<8u, 2, 7, exception_directory_errc::invalid_offset>(offset);
}

std::uint16_t alloc_m::get_allocation_size() const noexcept
{
	return static_cast<std::uint16_t>(get_value<5, 15>()) * 16u;
}

void alloc_m::set_allocation_size(std::uint16_t size)
{
	set_scaled_value<16u, 5, 15, exception_directory_errc::invalid_allocation_size>(size);
}

std::uint16_t save_regp::get_offset() const noexcept
{
	return static_cast<std::uint16_t>(get_value<10, 15>()) * 8u;
}

void save_regp::set_offset(std::uint16_t offset)
{
	set_scaled_value<8u, 10, 15, exception_directory_errc::invalid_offset>(offset);
}

std::uint8_t save_regp::get_register_pair() const noexcept
{
	return get_value<6, 9>() + 19u;
}

void save_regp::set_register_pair(std::uint8_t reg)
{
	//x29:x30 is the last valid register pair
	if (reg > 29u || reg < 19u)
		throw pe_error(exception_directory_errc::invalid_register);

	set_scaled_value<0, 6, 9, exception_directory_errc::invalid_register>(reg - 19u);
}

std::uint16_t save_regp_x::get_offset() const noexcept
{
	return static_cast<std::uint16_t>(get_value<10, 15>() + 1u) * 8u;
}

void save_regp_x::set_offset(std::uint16_t offset)
{
	if (offset < 8u)
		throw pe_error(exception_directory_errc::invalid_offset);

	offset -= 8u;
	set_scaled_value<8u, 10, 15, exception_directory_errc::invalid_offset>(offset);
}

std::uint8_t save_regp_x::get_register_pair() const noexcept
{
	return get_value<6, 9>() + 19u;
}

void save_regp_x::set_register_pair(std::uint8_t reg)
{
	//x29:x30 is the last valid register pair
	if (reg > 29u || reg < 19u)
		throw pe_error(exception_directory_errc::invalid_register);

	set_scaled_value<0, 6, 9, exception_directory_errc::invalid_register>(reg - 19u);
}

std::uint16_t save_reg::get_offset() const noexcept
{
	return static_cast<std::uint16_t>(get_value<10, 15>()) * 8u;
}

void save_reg::set_offset(std::uint16_t offset)
{
	set_scaled_value<8u, 10, 15, exception_directory_errc::invalid_offset>(offset);
}

std::uint8_t save_reg::get_register() const noexcept
{
	return get_value<6, 9>() + 19u;
}

void save_reg::set_register(std::uint8_t reg)
{
	//x30 is the last valid register
	if (reg > 30u || reg < 19u)
		throw pe_error(exception_directory_errc::invalid_register);

	set_scaled_value<0, 6, 9, exception_directory_errc::invalid_register>(reg - 19u);
}

std::uint16_t save_reg_x::get_offset() const noexcept
{
	return static_cast<std::uint16_t>(get_value<11, 15>() + 1u) * 8u;
}

void save_reg_x::set_offset(std::uint16_t offset)
{
	if (offset < 8u)
		throw pe_error(exception_directory_errc::invalid_offset);

	offset -= 8u;
	set_scaled_value<8u, 11, 15, exception_directory_errc::invalid_offset>(offset);
}

std::uint8_t save_reg_x::get_register() const noexcept
{
	return get_value<7, 10>() + 19u;
}

void save_reg_x::set_register(std::uint8_t reg)
{
	//x30 is the last valid register
	if (reg > 30u || reg < 19u)
		throw pe_error(exception_directory_errc::invalid_register);

	set_scaled_value<0, 7, 10, exception_directory_errc::invalid_register>(reg - 19u);
}

std::uint16_t save_lrpair::get_offset() const noexcept
{
	return static_cast<std::uint16_t>(get_value<10, 15>()) * 8u;
}

void save_lrpair::set_offset(std::uint16_t offset)
{
	set_scaled_value<8u, 10, 15, exception_directory_errc::invalid_offset>(offset);
}

std::uint8_t save_lrpair::get_register() const noexcept
{
	return 2u * get_value<7, 9>() + 19u;
}

void save_lrpair::set_register(std::uint8_t reg)
{
	//x30 is the last valid register
	if (reg > 30u || reg < 19u)
		throw pe_error(exception_directory_errc::invalid_register);

	reg -= 19u;
	set_scaled_value<2u, 7, 9, exception_directory_errc::invalid_register>(reg);
}

std::uint16_t save_fregp::get_offset() const noexcept
{
	return static_cast<std::uint16_t>(get_value<10, 15>()) * 8u;
}

void save_fregp::set_offset(std::uint16_t offset)
{
	set_scaled_value<8u, 10, 15, exception_directory_errc::invalid_offset>(offset);
}

std::uint8_t save_fregp::get_register_pair() const noexcept
{
	return get_value<7, 9>() + 8u;
}

void save_fregp::set_register_pair(std::uint8_t reg)
{
	//d30:d31 is the last valid register pair
	if (reg > 30u || reg < 8u)
		throw pe_error(exception_directory_errc::invalid_register);

	set_scaled_value<0, 7, 9, exception_directory_errc::invalid_register>(reg - 8u);
}

std::uint16_t save_fregp_x::get_offset() const noexcept
{
	return static_cast<std::uint16_t>(get_value<10, 15>() + 1u) * 8u;
}

void save_fregp_x::set_offset(std::uint16_t offset)
{
	if (offset < 8u)
		throw pe_error(exception_directory_errc::invalid_offset);

	offset -= 8u;
	set_scaled_value<8u, 10, 15, exception_directory_errc::invalid_offset>(offset);
}

std::uint8_t save_fregp_x::get_register_pair() const noexcept
{
	return get_value<7, 9>() + 8u;
}

void save_fregp_x::set_register_pair(std::uint8_t reg)
{
	//d30:d31 is the last valid register pair
	if (reg > 30u || reg < 8u)
		throw pe_error(exception_directory_errc::invalid_register);

	set_scaled_value<0, 7, 9, exception_directory_errc::invalid_register>(reg - 8u);
}

std::uint16_t save_freg::get_offset() const noexcept
{
	return static_cast<std::uint16_t>(get_value<10, 15>()) * 8u;
}

void save_freg::set_offset(std::uint16_t offset)
{
	set_scaled_value<8u, 10, 15, exception_directory_errc::invalid_offset>(offset);
}

std::uint8_t save_freg::get_register() const noexcept
{
	return get_value<7, 9>() + 8u;
}

void save_freg::set_register_pair(std::uint8_t reg)
{
	//d31 is the last valid register
	if (reg > 31u || reg < 8u)
		throw pe_error(exception_directory_errc::invalid_register);

	set_scaled_value<0, 7, 9, exception_directory_errc::invalid_register>(reg - 8u);
}

std::uint16_t save_freg_x::get_offset() const noexcept
{
	return static_cast<std::uint16_t>(get_value<11, 15>() + 1u) * 8u;
}

void save_freg_x::set_offset(std::uint16_t offset)
{
	if (offset < 8u)
		throw pe_error(exception_directory_errc::invalid_offset);

	offset -= 8u;
	set_scaled_value<8u, 11, 15, exception_directory_errc::invalid_offset>(offset);
}

std::uint8_t save_freg_x::get_register() const noexcept
{
	return get_value<8, 10>() + 8u;
}

void save_freg_x::set_register_pair(std::uint8_t reg)
{
	//d31 is the last valid register
	if (reg > 31u || reg < 8u)
		throw pe_error(exception_directory_errc::invalid_register);

	set_scaled_value<0, 8, 10, exception_directory_errc::invalid_register>(reg - 8u);
}

std::uint32_t alloc_l::get_allocation_size() const noexcept
{
	return static_cast<std::uint16_t>(get_value<8, 31>()) * 16u;
}

void alloc_l::set_allocation_size(std::uint32_t size)
{
	set_scaled_value<16u, 8, 31, exception_directory_errc::invalid_allocation_size>(size);
}

std::uint16_t add_fp::get_delta() const noexcept
{
	return static_cast<std::uint16_t>(get_value<8, 15>()) * 8u;
}

void add_fp::set_delta(std::uint16_t delta)
{
	set_scaled_value<8u, 8, 15, exception_directory_errc::invalid_delta>(delta);
}

custom_stack_case reserved_custom_stack::get_custom_stack_case() const noexcept
{
	return static_cast<custom_stack_case>(get_value<5, 7>());
}

void reserved_custom_stack::set_custom_stack_case(custom_stack_case value)
{
	set_scaled_value<0, 5, 7, exception_directory_errc::invalid_custom_stack_case>(
		static_cast<std::uint8_t>(value));
}

bool save_reg_any::is_reg_pair() const noexcept
{
	return get_value<9, 9>() != 0u;
}

void save_reg_any::set_reg_pair(bool is_reg_pair)
{
	set_value<9, 9>(is_reg_pair ? 1u : 0u);
}

bool save_reg_any::is_negative_offset() const noexcept
{
	return get_value<10, 10>() != 0u;
}

void save_reg_any::set_negative_offset(bool is_negative_offset)
{
	set_value<10, 10>(is_negative_offset ? 1u : 0u);
}

std::uint16_t save_reg_any::get_register_or_register_pair() const noexcept
{
	return get_value<11, 15>();
}

void save_reg_any::set_register_or_register_pair(std::uint16_t value)
{
	set_scaled_value<0, 11, 15, exception_directory_errc::invalid_register>(value);
}

register_character save_reg_any::get_register_character() const noexcept
{
	return static_cast<register_character>(get_value<16, 17>());
}

void save_reg_any::set_register_character(register_character character)
{
	set_scaled_value<0, 16, 17, exception_directory_errc::invalid_register>(
		static_cast<std::uint8_t>(character));
}

std::uint16_t save_reg_any::get_offset() const noexcept
{
	if (is_negative_offset())
		return (get_value<18, 23>() + 1u) * 16u;
	else
		return get_value<18, 23>() * 16u;
}

void save_reg_any::set_offset(std::uint16_t offset)
{
	if (is_negative_offset())
	{
		if (offset < 16u)
			throw pe_error(exception_directory_errc::invalid_offset);

		offset -= 16u;
		set_scaled_value<16u, 18, 23, exception_directory_errc::invalid_offset>(offset);
	}
	else
	{
		set_scaled_value<16u, 18, 23, exception_directory_errc::invalid_offset>(offset);
	}
}

} //namespace opcode

} //namespace pe_bliss::exceptions::arm64
