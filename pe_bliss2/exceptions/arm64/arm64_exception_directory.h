#pragma once

#include <cstddef>
#include <cstdint>
#include <list>
#include <system_error>
#include <type_traits>
#include <variant>

#include <boost/endian/conversion.hpp>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/detail/exceptions/image_runtime_function_entry.h"
#include "pe_bliss2/exceptions/arm_common/arm_common_unwind_info.h"

namespace pe_bliss::exceptions::arm64
{

enum class exception_directory_errc
{
	invalid_function_length = 1,
	invalid_frame_size,
	invalid_reg_int_value,
	invalid_reg_fp_value,
	invalid_version,
	unknown_unwind_code,
	invalid_allocation_size,
	invalid_offset,
	invalid_register,
	invalid_custom_stack_case,
	invalid_delta
};

} //namespace pe_bliss::exceptions::arm64

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::exceptions::arm64::exception_directory_errc> : true_type {};
} //namespace std

namespace pe_bliss::exceptions::arm64
{

std::error_code make_error_code(exception_directory_errc) noexcept;

class packed_unwind_data
{
public:
	enum class flag : std::uint8_t
	{
		//Packed unwind data used with a single
		//prolog and epilog at the beginning and end of the scope.
		packed_unwind_function = static_cast<std::uint8_t>(detail::exceptions::
			arm64_fnpdata_flags::pdata_packed_unwind_function),
		//Packed unwind data used for code without any prolog and epilog.
		//Useful for describing separated function segments.
		packed_unwind_fragment = static_cast<std::uint8_t>(detail::exceptions::
			arm64_fnpdata_flags::pdata_packed_unwind_fragment)
	};

	enum class cr : std::uint8_t
	{
		//Unchained function, <x29, lr> pair is not saved in stack.
		unchained = static_cast<std::uint8_t>(detail::exceptions::
			arm64_fnpdata_cr::pdata_cr_unchained),
		//Unchained function, <lr> is saved in stack
		unchained_saved_lr = static_cast<std::uint8_t>(detail::exceptions::
			arm64_fnpdata_cr::pdata_cr_unchained_saved_lr),
		//Reserved
		chained_with_pac = static_cast<std::uint8_t>(detail::exceptions::
			arm64_fnpdata_cr::pdata_cr_chained_with_pac),
		//chained function, a store/load pair instruction is used in prolog/epilog <x29, lr>
		chained = static_cast<std::uint8_t>(detail::exceptions::
			arm64_fnpdata_cr::pdata_cr_chained)
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
	flag get_flag() const noexcept
	{
		return static_cast<flag>(unwind_data_ & 0b11u);
	}
	
	//The length of the entire function in bytes, divided by 4
	[[nodiscard]]
	std::uint16_t get_function_length() const noexcept
	{
		return static_cast<std::uint16_t>(((unwind_data_ & 0x1ffcu) >> 2u) * 4u);
	}

	//The number of bytes of stack that is allocated for this function, divided by 16.
	[[nodiscard]]
	std::uint16_t get_frame_size() const noexcept
	{
		return static_cast<std::uint16_t>(((unwind_data_ & 0xff800000u) >> 23u) * 16u);
	}

	[[nodiscard]]
	cr get_cr() const noexcept
	{
		return static_cast<cr>((unwind_data_ & 0x600000u) >> 21u);
	}

	//Whether the function homes the integer parameter registers (x0-x7)
	//by storing them at the very start of the function.
	[[nodiscard]]
	bool homes_integer_parameter_registers() const noexcept
	{
		return (unwind_data_ & 0x100000u) != 0u;
	}

	//The number of non-volatile INT registers (x19-x28) saved in the canonical stack location.
	[[nodiscard]]
	std::uint8_t get_reg_int() const noexcept
	{
		return static_cast<std::uint8_t>((unwind_data_ & 0xf0000u) >> 16u);
	}
	
	//The number of non-volatile FP registers (d8-d15) saved in the canonical stack location.
	//(RegF=0: no FP register is saved; RegF>0: RegF+1 FP registers are saved).
	[[nodiscard]]
	std::uint8_t get_reg_fp() const noexcept
	{
		auto result = static_cast<std::uint8_t>((unwind_data_ & 0xe000u) >> 13u);
		return result ? result + 1u : 0u;
	}

public:
	void set_flag(flag value) noexcept;
	void set_function_length(std::uint16_t length);
	void set_frame_size(std::uint16_t size);
	void set_cr(cr value) noexcept;
	void set_homes_integer_parameter_registers(bool value) noexcept;
	void set_reg_int(std::uint8_t value);
	void set_reg_fp(std::uint8_t value);

private:
	std::uint32_t unwind_data_;
};

using epilog_info = arm_common::epilog_info<false>;

enum class unwind_code : std::uint8_t
{
	alloc_s,
	save_r19r20_x,
	save_fplr,
	save_fplr_x,
	alloc_m,
	save_regp,
	save_regp_x,
	save_reg,
	save_reg_x,
	save_lrpair,
	save_fregp,
	save_fregp_x,
	save_freg,
	save_freg_x,
	alloc_l,
	set_fp,
	add_fp,
	nop,
	end,
	end_c,
	save_next,
	save_reg_any,
	reserved_custom_stack,
	reserved2
};

[[nodiscard]]
unwind_code decode_unwind_code(std::byte value);

template<std::size_t Length>
using unwind_code_common = arm_common::unwind_code_common<Length>;

template<unwind_code Opcode>
class unwind_code_id
{
public:
	static constexpr auto opcode = Opcode;
};

namespace opcode
{

//000xxxxx: allocate small stack with size < 512 (2^5 * 16).
class alloc_s
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::alloc_s>
{
public:
	[[nodiscard]]
	std::uint16_t get_allocation_size() const noexcept;
	void set_allocation_size(std::uint16_t size);
};

//001zzzzz: save <x19,x20> pair at [sp-#Z*8]!, pre-indexed offset >= -248
class save_r19r20_x
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::save_r19r20_x>
{
public:
	[[nodiscard]]
	std::uint16_t get_offset() const noexcept;
	void set_offset(std::uint16_t offset);
};

//01zzzzzz: save <x29,lr> pair at [sp+#Z*8], offset <= 504.
class save_fplr
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::save_fplr>
{
public:
	[[nodiscard]]
	std::uint16_t get_offset() const noexcept;
	void set_offset(std::uint16_t offset);
};

//10zzzzzz: save <x29,lr> pair at [sp-(#Z+1)*8]!, pre-indexed offset >= -512
class save_fplr_x
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::save_fplr_x>
{
public:
	[[nodiscard]]
	std::uint16_t get_offset() const noexcept;
	void set_offset(std::uint16_t offset);
};

//11000xxx'xxxxxxxx: allocate large stack with size < 16k (2^11 * 16).
class alloc_m
	: public unwind_code_common<2u>
	, public unwind_code_id<unwind_code::alloc_m>
{
public:
	[[nodiscard]]
	std::uint16_t get_allocation_size() const noexcept;
	void set_allocation_size(std::uint16_t size);
};

//110010xx'xxzzzzzz: save x(19+#X) pair at [sp+#Z*8], offset <= 504
class save_regp
	: public unwind_code_common<2u>
	, public unwind_code_id<unwind_code::save_regp>
{
public:
	[[nodiscard]]
	std::uint16_t get_offset() const noexcept;
	void set_offset(std::uint16_t offset);

	[[nodiscard]]
	std::uint8_t get_register_pair() const noexcept;
	void set_register_pair(std::uint8_t reg);
};

//110011xx'xxzzzzzz: save pair x(19+#X) at [sp-(#Z+1)*8]!, pre-indexed offset >= -512
class save_regp_x
	: public unwind_code_common<2u>
	, public unwind_code_id<unwind_code::save_regp_x>
{
public:
	[[nodiscard]]
	std::uint16_t get_offset() const noexcept;
	void set_offset(std::uint16_t offset);

	[[nodiscard]]
	std::uint8_t get_register_pair() const noexcept;
	void set_register_pair(std::uint8_t reg);
};

//110100xx'xxzzzzzz: save reg x(19+#X) at [sp+#Z*8], offset <= 504
class save_reg
	: public unwind_code_common<2u>
	, public unwind_code_id<unwind_code::save_reg>
{
public:
	[[nodiscard]]
	std::uint16_t get_offset() const noexcept;
	void set_offset(std::uint16_t offset);

	[[nodiscard]]
	std::uint8_t get_register() const noexcept;
	void set_register(std::uint8_t reg);
};

//1101010x'xxxzzzzz: save reg x(19+#X) at [sp-(#Z+1)*8]!, pre-indexed offset >= -256
class save_reg_x
	: public unwind_code_common<2u>
	, public unwind_code_id<unwind_code::save_reg_x>
{
public:
	[[nodiscard]]
	std::uint16_t get_offset() const noexcept;
	void set_offset(std::uint16_t offset);

	[[nodiscard]]
	std::uint8_t get_register() const noexcept;
	void set_register(std::uint8_t reg);
};

//1101011x'xxzzzzzz: save pair <x(19+2*#X),lr> at [sp+#Z*8], offset <= 504
class save_lrpair
	: public unwind_code_common<2u>
	, public unwind_code_id<unwind_code::save_lrpair>
{
public:
	[[nodiscard]]
	std::uint16_t get_offset() const noexcept;
	void set_offset(std::uint16_t offset);

	[[nodiscard]]
	std::uint8_t get_register() const noexcept;
	void set_register(std::uint8_t reg);
};

//1101100x'xxzzzzzz: save pair d(8+#X) at [sp+#Z*8], offset <= 504
class save_fregp
	: public unwind_code_common<2u>
	, public unwind_code_id<unwind_code::save_fregp>
{
public:
	[[nodiscard]]
	std::uint16_t get_offset() const noexcept;
	void set_offset(std::uint16_t offset);

	[[nodiscard]]
	std::uint8_t get_register_pair() const noexcept;
	void set_register_pair(std::uint8_t reg);
};

//1101101x'xxzzzzzz: save pair d(8+#X), at [sp-(#Z+1)*8]!, pre-indexed offset >= -512
class save_fregp_x
	: public unwind_code_common<2u>
	, public unwind_code_id<unwind_code::save_fregp_x>
{
public:
	[[nodiscard]]
	std::uint16_t get_offset() const noexcept;
	void set_offset(std::uint16_t offset);

	[[nodiscard]]
	std::uint8_t get_register_pair() const noexcept;
	void set_register_pair(std::uint8_t reg);
};

//1101110x'xxzzzzzz: save reg d(8+#X) at [sp+#Z*8], offset <= 504
class save_freg
	: public unwind_code_common<2u>
	, public unwind_code_id<unwind_code::save_freg>
{
public:
	[[nodiscard]]
	std::uint16_t get_offset() const noexcept;
	void set_offset(std::uint16_t offset);

	[[nodiscard]]
	std::uint8_t get_register() const noexcept;
	void set_register_pair(std::uint8_t reg);
};

//11011110'xxxzzzzz: save reg d(8+#X) at [sp-(#Z+1)*8]!, pre-indexed offset >= -256
class save_freg_x
	: public unwind_code_common<2u>
	, public unwind_code_id<unwind_code::save_freg_x>
{
public:
	[[nodiscard]]
	std::uint16_t get_offset() const noexcept;
	void set_offset(std::uint16_t offset);

	[[nodiscard]]
	std::uint8_t get_register() const noexcept;
	void set_register_pair(std::uint8_t reg);
};

//11100000'xxxxxxxx'xxxxxxxx'xxxxxxxx: allocate large stack with size < 256M (2^24 *16)
class alloc_l
	: public unwind_code_common<4u>
	, public unwind_code_id<unwind_code::alloc_l>
{
public:
	[[nodiscard]]
	std::uint32_t get_allocation_size() const noexcept;
	void set_allocation_size(std::uint32_t size);
};

//11100001: set up x29: with: mov x29,sp
class set_fp
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::set_fp>
{
};

//11100010'xxxxxxxx: set up x29 with: add x29,sp,#x*8
class add_fp
	: public unwind_code_common<2u>
	, public unwind_code_id<unwind_code::add_fp>
{
public:
	[[nodiscard]]
	std::uint16_t get_delta() const noexcept;
	void set_delta(std::uint16_t delta);
};

//11100011: no unwind operation is required
class nop
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::nop>
{
};

//11100100: end of unwind code. Implies ret in epilog
class end
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::end>
{
};

//11100101: end of unwind code in current chained scope
class end_c
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::end_c>
{
};

//11100110: save next non-volatile Int or FP register pair
class save_next
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::save_next>
{
};

//XXX: undocumented, reverse engineered.
//MSDN marks this code as "reserved".
enum class register_character : std::uint8_t
{
	x = 0b00,
	d = 0b01,
	q = 0b10,
	undefined = 0b11
};

class save_reg_any
	: public unwind_code_common<3u>
	, public unwind_code_id<unwind_code::save_reg_any>
{
public:
	[[nodiscard]]
	bool is_reg_pair() const noexcept;
	void set_reg_pair(bool is_reg_pair);

	[[nodiscard]]
	bool is_negative_offset() const noexcept;
	void set_negative_offset(bool is_negative_offset);

	[[nodiscard]]
	std::uint16_t get_register_or_register_pair() const noexcept;
	void set_register_or_register_pair(std::uint16_t value);

	[[nodiscard]]
	register_character get_register_character() const noexcept;
	void set_register_character(register_character character);

	[[nodiscard]]
	std::uint16_t get_offset() const noexcept;
	void set_offset(std::uint16_t offset);
};

enum class custom_stack_case : std::uint8_t
{
	msft_op_trap_frame = 0b000,
	msft_op_machine_frame = 0b001,
	msft_op_context = 0b010,
	msft_op_clear_unwound_to_call = 0b100
};

//11101xxx: reserved for custom stack cases below only generated for asm routines
//11101000: Custom stack for MSFT_OP_TRAP_FRAME
//11101001: Custom stack for MSFT_OP_MACHINE_FRAME
//11101010: Custom stack for MSFT_OP_CONTEXT
//11101100: Custom stack for MSFT_OP_CLEAR_UNWOUND_TO_CALL
class reserved_custom_stack
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::reserved_custom_stack>
{
public:
	[[nodiscard]]
	custom_stack_case get_custom_stack_case() const noexcept;
	void set_custom_stack_case(custom_stack_case value);
};

//1111xxxx: reserved
class reserved2
	: public unwind_code_common<1u>
	, public unwind_code_id<unwind_code::reserved2>
{
};

} //namespace opcode

struct unwind_record_options
{
	using unwind_code_type = std::variant<
		opcode::alloc_s,
		opcode::save_r19r20_x,
		opcode::save_fplr,
		opcode::save_fplr_x,
		opcode::alloc_m,
		opcode::save_regp,
		opcode::save_regp_x,
		opcode::save_reg,
		opcode::save_reg_x,
		opcode::save_lrpair,
		opcode::save_fregp,
		opcode::save_fregp_x,
		opcode::save_freg,
		opcode::save_freg_x,
		opcode::alloc_l,
		opcode::set_fp,
		opcode::add_fp,
		opcode::nop,
		opcode::end,
		opcode::end_c,
		opcode::save_next,
		opcode::save_reg_any,
		opcode::reserved_custom_stack,
		opcode::reserved2
	>;

	static constexpr std::uint32_t function_length_multiplier = 4u;
	static constexpr bool has_f_bit = false;
};

using extended_unwind_record = arm_common::extended_unwind_record<epilog_info, unwind_record_options>;

template<typename... Bases>
using runtime_function_base = arm_common::runtime_function_base<
	detail::exceptions::image_arm64_runtime_function_entry, packed_unwind_data, extended_unwind_record,
	Bases...>;

template<typename... Bases>
using exception_directory_base = arm_common::exception_directory_base<runtime_function_base, Bases...>;

using runtime_function = runtime_function_base<>;
using runtime_function_details = runtime_function_base<error_list>;

using exception_directory = exception_directory_base<>;
using exception_directory_details = exception_directory_base<error_list>;

} //namespace pe_bliss::exceptions::arm64
