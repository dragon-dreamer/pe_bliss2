#pragma once

#include <bit>
#include <cstdint>
#include <list>
#include <optional>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/detail/exceptions/image_runtime_function_entry.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/exceptions/arm_common/arm_common_unwind_info.h"
#include "pe_bliss2/pe_error.h"

#include "utilities/static_class.h"

namespace pe_bliss::exceptions::arm
{

enum class exception_directory_errc
{
	invalid_function_length = 1,
	invalid_non_volatile_register_count,
	invalid_stack_adjust_value,
	invalid_stack_adjust_flags,
	unknown_unwind_code,
	invalid_allocation_size,
	invalid_delta,
	invalid_registers
};

} //namespace pe_bliss::exceptions::arm

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::exceptions::arm::exception_directory_errc> : true_type {};
} //namespace std

namespace pe_bliss::exceptions::arm
{

using epilog_info = arm_common::epilog_info<true>;

enum class unwind_code
{
	alloc_s,
	save_r0r12_lr,
	mov_sprx,
	save_r4rx_lr,
	save_r4rx_lr_wide,
	save_d8dx,
	alloc_s_wide,
	save_r0r7_lr,
	ms_specific,
	ldr_lr_sp,
	save_dsde,
	save_dsde_16,
	alloc_m,
	alloc_m_wide,
	alloc_l,
	alloc_l_wide,
	nop,
	nop_wide,
	end_nop,
	end_nop_wide,
	end,
	reserved //F0-F4
};

template<std::size_t Length>
using unwind_code_common = arm_common::unwind_code_common<Length>;

template<unwind_code Opcode, std::size_t Thumb2OpcodeBitSize>
class unwind_code_id
{
public:
	static constexpr auto opcode = Opcode;
	static constexpr auto thumb2_opcode_bit_size = Thumb2OpcodeBitSize;
};

[[nodiscard]]
unwind_code decode_unwind_code(std::byte value);

namespace opcode
{

//add sp,sp,#X
class alloc_s
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::alloc_s, 16u>
{
public:
	[[nodiscard]]
	std::uint16_t get_allocation_size() const noexcept;

	void set_allocation_size(std::uint16_t size);
};

struct int_registers final : utilities::static_class
{
	enum value : std::uint16_t
	{
		r0 = 1u << 0u,
		r1 = 1u << 1u,
		r2 = 1u << 2u,
		r3 = 1u << 3u,
		r4 = 1u << 4u,
		r5 = 1u << 5u,
		r6 = 1u << 6u,
		r7 = 1u << 7u,
		r8 = 1u << 8u,
		r9 = 1u << 9u,
		r10 = 1u << 10u,
		r11 = 1u << 11u,
		r12 = 1u << 12u,
		lr = 1u << 13u
	};
};

struct fp_registers final : utilities::static_class
{
	enum value : std::uint16_t
	{
		d0 = 1u << 0u,
		d1 = 1u << 1u,
		d2 = 1u << 2u,
		d3 = 1u << 3u,
		d4 = 1u << 4u,
		d5 = 1u << 5u,
		d6 = 1u << 6u,
		d7 = 1u << 7u,
		d8 = 1u << 8u,
		d9 = 1u << 9u,
		d10 = 1u << 10u,
		d11 = 1u << 11u,
		d12 = 1u << 12u,
		d13 = 1u << 13u,
		d14 = 1u << 14u,
		d15 = 1u << 15u
	};
};

enum fp_register : std::uint8_t
{
	d0,
	d1,
	d2,
	d3,
	d4,
	d5,
	d6,
	d7,
	d8,
	d9,
	d10,
	d11,
	d12,
	d13,
	d14,
	d15
};

//pop {r0-r12, lr}
class save_r0r12_lr
	: public unwind_code_common<2u>
	, public unwind_code_id<unwind_code::save_r0r12_lr, 32u>
{
public:
	[[nodiscard]]
	int_registers::value get_saved_registers() const noexcept;
	
	void set_saved_registers(int_registers::value value);
};

//mov sp,rX
class mov_sprx
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::mov_sprx, 16u>
{
public:
	[[nodiscard]]
	std::uint8_t get_delta() const noexcept;
	
	void set_delta(std::uint8_t delta);
};

template<std::uint32_t Delta, std::uint32_t ForbiddenMask, std::uint32_t RequiredMask>
class save_r4rx_lr_base
	: public unwind_code_common<1u>
{
public:
	[[nodiscard]]
	int_registers::value get_saved_registers() const noexcept
	{
		std::uint32_t max_register = get_value<6, 7>() + Delta;
		auto register_mask = ((1u << (max_register + 1u)) - 1u) & 0x1ff0u;
		if (get_value<5, 5>())
			register_mask |= int_registers::lr;
		return static_cast<int_registers::value>(register_mask);
	}

	void set_saved_registers(int_registers::value value)
	{
		auto register_mask = static_cast<std::uint32_t>(value);
		if (register_mask & ForbiddenMask)
			throw pe_error(exception_directory_errc::invalid_registers);
		if ((register_mask & RequiredMask) != RequiredMask)
			throw pe_error(exception_directory_errc::invalid_registers);
		std::uint16_t opcode_value = 0u;
		if (register_mask & static_cast<std::uint32_t>(int_registers::lr))
		{
			opcode_value |= 0x4u;
			register_mask &= ~static_cast<std::uint32_t>(int_registers::lr);
		}

		auto max_register = std::bit_width(register_mask);
		if (max_register)
			opcode_value |= (max_register - Delta - 1u);

		set_value<5, 7>(opcode_value);
	}
};

//pop {r4-rX,lr}
class save_r4rx_lr
	: public save_r4rx_lr_base<4u, 0xdf0fu, 0x10u>
	, public unwind_code_id<unwind_code::save_r4rx_lr, 16u>
{
};

//pop {r4-rX,lr}
class save_r4rx_lr_wide
	: public save_r4rx_lr_base<8u, 0xd00fu, 0x1f0u>
	, public unwind_code_id<unwind_code::save_r4rx_lr_wide, 32u>
{
};

//add sp,sp,#X
class save_d8dx
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::save_d8dx, 32u>
{
public:
	[[nodiscard]]
	fp_registers::value get_saved_registers() const noexcept;

	void set_saved_registers(fp_registers::value value);
};

//add sp,sp,#X
class alloc_s_wide
	: public unwind_code_common<2u>
	, public unwind_code_id<unwind_code::alloc_s_wide, 32u>
{
public:
	[[nodiscard]]
	std::uint16_t get_allocation_size() const noexcept;

	void set_allocation_size(std::uint16_t size);
};

//pop {r0-r7,lr}
class save_r0r7_lr
	: public unwind_code_common<2u>
	, public unwind_code_id<unwind_code::save_r0r7_lr, 16u>
{
public:
	[[nodiscard]]
	int_registers::value get_saved_registers() const noexcept;

	void set_saved_registers(int_registers::value value);
};

class ms_specific
	: public unwind_code_common<2u>
	, public unwind_code_id<unwind_code::ms_specific, 16u>
{
public:
	[[nodiscard]]
	bool is_available_opcode() const noexcept;
};

//ldr lr,[sp],#X
class ldr_lr_sp
	: public unwind_code_common<2u>
	, public unwind_code_id<unwind_code::ldr_lr_sp, 32u>
{
public:
	[[nodiscard]]
	bool is_available_opcode() const noexcept;

	[[nodiscard]]
	std::uint8_t get_delta() const noexcept;

	void set_delta(std::uint8_t delta);
};

template<std::uint8_t Delta>
class save_dsde_base
	: public unwind_code_common<2u>
{
public:
	[[nodiscard]]
	std::pair<fp_register, fp_register> get_saved_registers_range() const noexcept
	{
		return { static_cast<fp_register>(get_value<8, 11>() + Delta),
			static_cast<fp_register>(get_value<12, 15>() + Delta) };
	}

	void set_saved_registers_range(const std::pair<fp_register, fp_register>& value)
	{
		auto s = static_cast<std::uint8_t>(value.first);
		auto e = static_cast<std::uint8_t>(value.second);
		if constexpr (Delta)
		{
			if (s < Delta || e < Delta)
				throw pe_error(exception_directory_errc::invalid_registers);
		}
		if (s > e)
			throw pe_error(exception_directory_errc::invalid_registers);

		set_value<8, 11>(s);
		set_value<12, 15>(e);
	}
};

//vpop {dS-dE}
class save_dsde
	: public save_dsde_base<0u>
	, public unwind_code_id<unwind_code::save_dsde, 32u>
{
};

//vpop {dS-dE}
class save_dsde_16
	: public save_dsde_base<16u>
	, public unwind_code_id<unwind_code::save_dsde_16, 32u>
{
};

class alloc_m_base
	: public unwind_code_common<3u>
{
public:
	[[nodiscard]]
	std::uint32_t get_allocation_size() const noexcept;

	void set_allocation_size(std::uint32_t size);
};

class alloc_l_base
	: public unwind_code_common<4u>
{
public:
	[[nodiscard]]
	std::uint32_t get_allocation_size() const noexcept;

	void set_allocation_size(std::uint32_t size);
};

//add sp,sp,#X
class alloc_m
	: public alloc_m_base
	, public unwind_code_id<unwind_code::alloc_m, 16u>
{
};

//add sp,sp,#X
class alloc_m_wide
	: public alloc_m_base
	, public unwind_code_id<unwind_code::alloc_m_wide, 32u>
{
};

//add sp,sp,#X
class alloc_l
	: public alloc_l_base
	, public unwind_code_id<unwind_code::alloc_l, 16u>
{
};

//add sp,sp,#X
class alloc_l_wide
	: public alloc_l_base
	, public unwind_code_id<unwind_code::alloc_l_wide, 32u>
{
};

class nop
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::nop, 16u>
{
};

class nop_wide
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::nop_wide, 32u>
{
};

class end_nop
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::end_nop, 16u>
{
};

class end_nop_wide
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::end_nop_wide, 32u>
{
};

class end
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::end, 0u>
{
};

class reserved
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::reserved, 0u>
{
};

} //namespace opcode

struct unwind_record_options
{
	using unwind_code_type = std::variant<
		opcode::alloc_s,
		opcode::save_r0r12_lr,
		opcode::mov_sprx,
		opcode::save_r4rx_lr,
		opcode::save_r4rx_lr_wide,
		opcode::save_d8dx,
		opcode::alloc_s_wide,
		opcode::save_r0r7_lr,
		opcode::ms_specific,
		opcode::ldr_lr_sp,
		opcode::save_dsde,
		opcode::save_dsde_16,
		opcode::alloc_m,
		opcode::alloc_m_wide,
		opcode::alloc_l,
		opcode::alloc_l_wide,
		opcode::nop,
		opcode::nop_wide,
		opcode::end_nop,
		opcode::end_nop_wide,
		opcode::end,
		opcode::reserved
	>;

	static constexpr std::uint32_t function_length_multiplier = 2u;
	static constexpr bool has_f_bit = true;
};

using extended_unwind_record = arm_common::extended_unwind_record<epilog_info, unwind_record_options>;

struct stack_adjust_flags
{
	std::uint8_t stack_adjustment_words_number{};
	bool prologue_folding{};
	bool epilogue_folding{};
};

class packed_unwind_data
{
public:
	enum class flag : std::uint8_t
	{
		//Packed unwind data.
		packed_unwind_function = 0b01u,
		//Pcked unwind data where the function is assumed to have no prologue.
		//This is useful for describing function fragments that are discontiguous with the start of the function.
		packed_unwind_fragment = 0b10u
	};

	enum class ret : std::uint8_t
	{
		//Return via pop {pc} (the L flag bit must be set to 1 in this case).
		pop_pc = 0b00u,
		//Return by using a 16-bit branch.
		branch_16bit = 0b01u,
		//Return by using a 32-bit branch.
		branch_32bit = 0b10u,
		//No epilogue at all. This is useful for describing a discontiguous function fragment that may only contain a prologue, but whose epilogue is elsewhere.
		no_epilogue = 0b11u
	};

	enum class saved_non_volatile_registers : std::uint8_t
	{
		integer,
		floating_point,
		none
	};

public:
	constexpr explicit packed_unwind_data(std::uint32_t unwind_data) noexcept
		: unwind_data_(unwind_data)
	{
	}

	[[nodiscard]]
	std::uint32_t get_packed_value() const noexcept
	{
		return unwind_data_;
	}

public:
	[[nodiscard]]
	flag get_flag() const noexcept;

	//The length of the entire function in bytes divided by 2.
	[[nodiscard]]
	std::uint16_t get_function_length() const noexcept;

	[[nodiscard]]
	ret get_ret() const noexcept;

	//Whether the function homes the integer parameter registers (r0-r3)
	//by pushing them at the start of the function, and deallocates the 16 bytes of stack.
	[[nodiscard]]
	bool homes_integer_parameter_registers() const noexcept;

	//The index of the last saved non-volatile register.
	//If the R bit is 0, then only integer registers are being saved,
	//and are assumed to be in the range of r4-rN, where N is equal to 4 + Reg.
	//If the R bit is 1, then only floating-point registers are being saved,
	//and are assumed to be in the range of d8-dN, where N is equal to 8 + Reg.
	//The special combination of R = 1 and Reg = 7 indicates that no registers are saved.
	[[nodiscard]]
	std::uint8_t get_last_saved_non_volatile_register_index() const noexcept;

	//Whether the saved non-volatile registers are integer registers (0) or floating-point registers (1).
	//If R is set to 1 and the Reg field is set to 7, no non-volatile registers were pushed.
	[[nodiscard]]
	saved_non_volatile_registers get_saved_non_volatile_registers() const noexcept;

	//Whether the function saves/restores LR, along with other registers indicated by the Reg field.
	[[nodiscard]]
	bool save_restore_lr() const noexcept;

	//Whether the function includes extra instructions to set up a frame
	//chain for fast stack walking (1) or not (0).
	//If this bit is set, r11 is implicitly added to the list of integer non-volatile registers saved.
	[[nodiscard]]
	bool includes_extra_instructions() const noexcept;

	//Stack Adjust is a 10-bit field that indicates the number of bytes of stack
	//that are allocated for this function, divided by 4.
	//However, only values between 0x000-0x3F3 can be directly encoded.
	//Functions that allocate more than 4044 bytes of stack must use a full .xdata record.
	//If the Stack Adjust field is 0x3F4 or larger, then the low 4 bits have special meaning:
	// - Bits 0-1 indicate the number of words of stack adjustment(1-4) minus 1.
	// - Bit 2 is set to 1 if the prologue combined this adjustment into its push operation.
	// - Bit 3 is set to 1 if the epilogue combined this adjustment into its pop operation.
	[[nodiscard]]
	std::uint16_t get_stack_adjust() const noexcept;

	[[nodiscard]]
	std::optional<stack_adjust_flags> get_stack_adjust_flags() const noexcept;

public:
	void set_flag(flag value) noexcept;
	void set_function_length(std::uint16_t value);
	void set_ret(ret value) noexcept;
	void set_homes_integer_parameter_registers(bool value) noexcept;
	void set_saved_non_volatile_registers(saved_non_volatile_registers value) noexcept;
	void set_last_saved_non_volatile_register_index(std::uint8_t index);
	void set_save_restore_lr(bool value) noexcept;
	void set_includes_extra_instructions(bool value) noexcept;
	void set_stack_adjust(std::uint16_t value);
	void set_stack_adjust_flags(const stack_adjust_flags& flags);

private:
	[[nodiscard]]
	std::uint16_t get_stack_adjust_raw() const noexcept;

private:
	std::uint32_t unwind_data_;
};

template<typename... Bases>
using runtime_function_base = arm_common::runtime_function_base<
	detail::exceptions::image_arm_runtime_function_entry, packed_unwind_data, extended_unwind_record,
	Bases...>;

template<typename... Bases>
using exception_directory_base = arm_common::exception_directory_base<runtime_function_base, Bases...>;

using runtime_function = runtime_function_base<>;
using runtime_function_details = runtime_function_base<error_list>;

using exception_directory = exception_directory_base<>;
using exception_directory_details = exception_directory_base<error_list>;

} //namespace pe_bliss::exceptions::arm
