#include "exceptions_dumper.h"

#include <cstddef>
#include <functional>
#include <iomanip>
#include <variant>

#include "formatter.h"

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/exceptions/exception_directory_loader.h"

namespace
{

void dump_flags_and_version(formatter& fmt, const pe_bliss::exceptions::x64::unwind_info& info,
	std::size_t left_padding)
{
	fmt.get_stream() << '\n';
	fmt.get_stream() << std::setw(left_padding) << std::setfill(' ') << "";

	{
		color_changer changer(fmt.get_stream(), fmt.get_color_provider(),
			fmt.flags_fg_color, fmt.flags_bg_color);
		fmt.get_stream() << "Version: " << std::dec
			<< static_cast<std::uint32_t>(info.get_version());
	}

	using enum pe_bliss::exceptions::x64::unwind_flags::value;
	fmt.print_flags(info.get_unwind_flags(), left_padding, {
		{ ehandler, "EHANDLER" },
		{ uhandler, "UHANDLER" },
		{ chaininfo, "CHAININFO" }
	});
}

const char* register_id_to_string(pe_bliss::exceptions::x64::register_id reg_id) noexcept
{
	using enum pe_bliss::exceptions::x64::register_id;
	switch (reg_id)
	{
	case rax: return "RAX";
	case rcx: return "RCX";
	case rdx: return "RDX";
	case rbx: return "RBX";
	case rsp: return "RSP";
	case rbp: return "RBP";
	case rsi: return "RSI";
	case rdi: return "RDI";
	case r8: return "R8";
	case r9: return "R9";
	case r10: return "R10";
	case r11: return "R11";
	case r12: return "R12";
	case r13: return "R13";
	case r14: return "R14";
	case r15: return "R15";
	default: return "Unknown";
	}
}

const char* opcode_id_to_string(pe_bliss::exceptions::x64::opcode_id opcode_id) noexcept
{
	using enum pe_bliss::exceptions::x64::opcode_id;
	switch (opcode_id)
	{
	case push_nonvol: return "PUSH_NONVOL";
	case alloc_large: return "ALLOC_LARGE";
	case alloc_small: return "ALLOC_SMALL";
	case set_fpreg: return "SET_FPREG";
	case save_nonvol: return "SAVE_NONVOL";
	case save_nonvol_far: return "SAVE_NONVOL_FAR";
	case epilog: return "EPILOG";
	case spare: return "SPARE";
	case save_xmm128: return "SAVE_XMM128";
	case save_xmm128_far: return "SAVE_XMM128_FAR";
	case push_machframe: return "PUSH_MACHFRAME";
	case set_fpreg_large: return "SET_FPREG_LARGE";
	default: return "Unknown";
	}
}

void dump_frame_register_and_offset(formatter& fmt, const pe_bliss::exceptions::x64::unwind_info& info,
	std::size_t left_padding)
{
	if (!info.get_frame_register())
		return;

	fmt.get_stream() << '\n';
	fmt.get_stream() << std::setw(left_padding) << std::setfill(' ') << "";

	color_changer changer(fmt.get_stream(), fmt.get_color_provider(),
	fmt.flags_fg_color, fmt.flags_bg_color);
	fmt.get_stream() << "FR: " << register_id_to_string(*info.get_frame_register())
		<< ", scaled offset: " << std::dec << static_cast<std::uint32_t>(info.get_scaled_frame_register_offset());
}

constexpr void dump_additional_info(formatter&, std::monostate, std::size_t)
{
}

void dump_additional_info(formatter& fmt,
	const pe_bliss::exceptions::x64::runtime_function_details::exception_handler_rva_type& rva,
	std::size_t)
{
	fmt.print_field_name("Handler RVA");
	fmt.print_offsets_and_value(rva, true);
	fmt.get_stream() << "\n\n";
}

void dump_runtime_function(formatter& fmt, const pe_bliss::exceptions::x64::runtime_function_details& func,
	std::size_t nested_level = 0);

void dump_additional_info(formatter& fmt,
	const pe_bliss::exceptions::x64::runtime_function_details::runtime_function_ptr& func_ptr,
	std::size_t nested_level)
{
	dump_runtime_function(fmt, *func_ptr, nested_level + 1u);
}

template<typename Opcode>
concept opcode_with_register = requires(Opcode opcode) { opcode.get_register(); };
template<typename Opcode>
concept opcode_with_allocation_size = requires(Opcode opcode) { opcode.get_allocation_size(); };
template<typename Opcode>
concept opcode_with_stack_offset = requires(Opcode opcode) { opcode.get_stack_offset(); };
template<typename Opcode>
concept opcode_with_delta = requires(Opcode opcode) { opcode.get_delta(); };
template<typename Opcode>
concept opcode_with_available_check = requires(Opcode opcode) { opcode.is_available_opcode(); };
template<typename Opcode>
concept opcode_with_registers_range = requires(Opcode opcode) { opcode.get_saved_registers_range(); };
template<typename Opcode>
concept opcode_with_saved_int_registers = requires(Opcode opcode) {
	{ opcode.get_saved_registers() } -> std::same_as<pe_bliss::exceptions::arm::opcode::int_registers::value>;
};
template<typename Opcode>
concept opcode_with_saved_fp_registers = requires(Opcode opcode) {
	{ opcode.get_saved_registers() } -> std::same_as<pe_bliss::exceptions::arm::opcode::fp_registers::value>;
};

template<typename Opcode>
void dump_opcode_id_and_register(formatter& fmt, const Opcode& opcode)
{
	fmt.get_stream() << " (";
	fmt.print_string(opcode_id_to_string(Opcode::opcode));
	fmt.get_stream() << ')';

	if constexpr (opcode_with_register<Opcode>)
	{
		fmt.get_stream() << ", ";
		fmt.print_field_name("Register");
		fmt.get_stream() << ": ";
		fmt.print_string(register_id_to_string(opcode.get_register()));
	}
}

template<typename Opcode>
constexpr void dump_unwind_code(formatter&, const Opcode&)
{
}

void dump_unwind_code(formatter& fmt, const opcode_with_allocation_size auto& opcode)
{
	fmt.print_field_name("Allocation size");
	fmt.get_stream() << ": ";
	fmt.print_value(opcode.get_allocation_size(), true);
	fmt.get_stream() << "\n\n";
}

void dump_unwind_code(formatter& fmt, const opcode_with_stack_offset auto& opcode)
{
	fmt.print_field_name("Stack offset");
	fmt.get_stream() << ": ";
	fmt.print_value(opcode.get_stack_offset(), true);
	fmt.get_stream() << "\n\n";
}

void dump_unwind_code(formatter& fmt, const pe_bliss::exceptions::x64::push_machframe& opcode)
{
	fmt.print_field_name("Push error code");
	fmt.get_stream() << ": ";
	fmt.print_string(opcode.push_error_code() ? "YES" : "NO");
	fmt.get_stream() << '\n';
	fmt.print_field_name("RSP decrement");
	fmt.get_stream() << ": ";
	fmt.print_value(opcode.get_rsp_decrement(), true);
	fmt.get_stream() << "\n\n";
}

void dump_runtime_function(formatter& fmt, const pe_bliss::exceptions::x64::runtime_function_details& func,
	std::size_t nested_level)
{
	std::string name("Runtime function descriptor");
	if (nested_level)
		name += " (chained #" + std::to_string(nested_level) + ")";
	fmt.print_structure(name.c_str(), func.get_descriptor(), std::array{
		value_info{"begin_address"},
		value_info{"end_address"},
		value_info{"unwind_info_address"}
	});

	fmt.print_errors(func);

	const auto& unwind_info = func.get_unwind_info();
	fmt.print_structure("Unwind info descriptor", unwind_info.get_descriptor(), std::array{
		value_info{"flags_and_version", true, std::bind(dump_flags_and_version,
			std::ref(fmt), std::cref(unwind_info), std::placeholders::_1)},
		value_info{"size_of_prolog"},
		value_info{"count_of_unwind_codes"},
		value_info{"frame_register_and_offset", true, std::bind(dump_frame_register_and_offset,
			std::ref(fmt), std::cref(unwind_info), std::placeholders::_1)}
	});

	if (!unwind_info.get_unwind_code_list().empty())
	{
		fmt.print_structure_name("Unwind codes");
		fmt.get_stream() << "\n\n";
	}

	for (const auto& opcode_variant : unwind_info.get_unwind_code_list())
	{
		std::visit([&fmt] (const auto& opcode) {
			fmt.print_structure("Opcode descriptor", opcode.get_descriptor(), std::array{
				value_info{"offset_in_prolog"},
				value_info{"unwind_operation_code_and_info", true,
					[&fmt, &opcode] (std::size_t) { dump_opcode_id_and_register(fmt, opcode); }},
				value_info{"node"}
			});
			dump_unwind_code(fmt, opcode);
		}, opcode_variant);
	}

	std::visit([&fmt, nested_level] (const auto& info) {
		dump_additional_info(fmt, info, nested_level);
	}, func.get_additional_info());
}

constexpr void dump_unwind_info(formatter&, std::monostate)
{
}

template<typename FlagEnum>
const char* packed_unwind_info_flag_to_string(FlagEnum flag) noexcept
{
	switch (flag)
	{
	case FlagEnum::packed_unwind_function:
		return "PACKED_UNWIND_FUNCTION";
	case FlagEnum::packed_unwind_fragment:
		return "PACKED_UNWIND_FRAGMENT";
	default:
		return "UNKNOWN";
	}
}

const char* packed_unwind_info_cr_to_string(pe_bliss::exceptions::arm64::packed_unwind_data::cr cr) noexcept
{
	using enum pe_bliss::exceptions::arm64::packed_unwind_data::cr;
	switch (cr)
	{
	case unchained:
		return "UNCHAINED";
	case unchained_saved_lr:
		return "UNCHAINED_SAVED_LR";
	case chained_with_pac:
		return "CHAINED_WITH_PAC";
	case chained:
		return "CHAINED";
	default:
		return "UNKNOWN";
	}
}

const char* packed_unwind_info_ret_to_string(pe_bliss::exceptions::arm::packed_unwind_data::ret ret) noexcept
{
	using enum pe_bliss::exceptions::arm::packed_unwind_data::ret;
	switch (ret)
	{
	case pop_pc:
		return "POP_PC";
	case branch_16bit:
		return "BRANCH_16BIT";
	case branch_32bit:
		return "BRANCH_32BIT";
	case no_epilogue:
		return "NO_EPILOGUE";
	default:
		return "UNKNOWN";
	}
}

template<typename PackedUnwindData>
void dump_arm_packed_unwind_data_common(formatter& fmt, const PackedUnwindData& data)
{
	fmt.print_structure_name("Packed unwind data");
	fmt.get_stream() << '\n';

	fmt.print_field_name("Flag");
	fmt.get_stream() << ": ";
	fmt.print_string(packed_unwind_info_flag_to_string(data.get_flag()));
	fmt.get_stream() << '\n';

	fmt.print_field_name("Function length");
	fmt.get_stream() << ": ";
	fmt.print_value(data.get_function_length(), true);
	fmt.get_stream() << '\n';

	fmt.print_field_name("Homes integer parameter registers");
	fmt.get_stream() << ": ";
	fmt.print_string(data.homes_integer_parameter_registers() ? "YES" : "NO");
	fmt.get_stream() << '\n';
}

void dump_unwind_info(formatter& fmt, const pe_bliss::exceptions::arm::packed_unwind_data& data)
{
	dump_arm_packed_unwind_data_common(fmt, data);

	fmt.print_field_name("RET");
	fmt.get_stream() << ": ";
	fmt.print_string(packed_unwind_info_ret_to_string(data.get_ret()));
	fmt.get_stream() << '\n';

	fmt.print_field_name("Save/restore LR");
	fmt.get_stream() << ": ";
	fmt.print_string(data.save_restore_lr() ? "YES" : "NO");
	fmt.get_stream() << '\n';

	fmt.print_field_name("Includes extra instructions");
	fmt.get_stream() << ": ";
	fmt.print_string(data.includes_extra_instructions() ? "YES" : "NO");
	fmt.get_stream() << '\n';

	fmt.print_field_name("Saved non-volatile registers");
	fmt.get_stream() << ": ";
	auto saved_non_volatile_registers = data.get_saved_non_volatile_registers();
	using enum pe_bliss::exceptions::arm::packed_unwind_data::saved_non_volatile_registers;
	if (saved_non_volatile_registers == none)
		fmt.print_string("None");
	else if (saved_non_volatile_registers == integer)
		fmt.print_string("Integer");
	else
		fmt.print_string("Floating-point");

	fmt.get_stream() << '\n';

	if (saved_non_volatile_registers != none)
	{
		fmt.print_field_name("Saved non-volatile registers");
		fmt.get_stream() << ": ";
		if (saved_non_volatile_registers == integer)
		{
			fmt.print_string(("r4-r"
				+ std::to_string(data.get_last_saved_non_volatile_register_index())).c_str());
		}
		else
		{
			fmt.print_string(("d8-d"
				+ std::to_string(data.get_last_saved_non_volatile_register_index())).c_str());
		}
		fmt.get_stream() << '\n';
	}

	fmt.print_field_name("Stack adjust");
	fmt.get_stream() << ": ";
	fmt.print_value(data.get_stack_adjust(), true);
	fmt.get_stream() << '\n';

	auto stack_adjust_flags = data.get_stack_adjust_flags();
	if (stack_adjust_flags)
	{
		fmt.print_field_name("Stack adjustment words number");
		fmt.get_stream() << ": ";
		fmt.print_value(stack_adjust_flags->stack_adjustment_words_number, true);
		fmt.get_stream() << '\n';

		fmt.print_field_name("Prologue folding");
		fmt.get_stream() << ": ";
		fmt.print_string(stack_adjust_flags->prologue_folding ? "YES" : "NO");
		fmt.get_stream() << '\n';

		fmt.print_field_name("Epilogue folding");
		fmt.get_stream() << ": ";
		fmt.print_string(stack_adjust_flags->epilogue_folding ? "YES" : "NO");
		fmt.get_stream() << '\n';
	}

	fmt.get_stream() << '\n';
}

void dump_unwind_info(formatter& fmt, const pe_bliss::exceptions::arm64::packed_unwind_data& data)
{
	dump_arm_packed_unwind_data_common(fmt, data);

	fmt.print_field_name("Frame size");
	fmt.get_stream() << ": ";
	fmt.print_value(data.get_frame_size(), true);
	fmt.get_stream() << '\n';

	fmt.print_field_name("CR");
	fmt.get_stream() << ": ";
	fmt.print_string(packed_unwind_info_cr_to_string(data.get_cr()));
	fmt.get_stream() << '\n';

	fmt.print_field_name("Number of non-volatile INT registers");
	fmt.get_stream() << ": ";
	fmt.print_value(data.get_reg_int(), true);
	fmt.get_stream() << '\n';

	fmt.print_field_name("Number of non-volatile FP registers");
	fmt.get_stream() << ": ";
	fmt.print_value(data.get_reg_fp(), true);
	fmt.get_stream() << "\n\n";
}

const char* opcode_id_to_string(pe_bliss::exceptions::arm64::unwind_code opcode_id) noexcept
{
	using enum pe_bliss::exceptions::arm64::unwind_code;
	switch (opcode_id)
	{
	case alloc_s: return "alloc_s";
	case save_r19r20_x: return "save_r19r20_x";
	case save_fplr: return "save_fplr";
	case save_fplr_x: return "save_fplr_x";
	case alloc_m: return "alloc_m";
	case save_regp: return "save_regp";
	case save_regp_x: return "save_regp_x";
	case save_reg: return "save_reg";
	case save_reg_x: return "save_reg_x";
	case save_lrpair: return "save_lrpair";
	case save_fregp: return "save_fregp";
	case save_fregp_x: return "save_fregp_x";
	case save_freg: return "save_freg";
	case save_freg_x: return "save_freg_x";
	case alloc_l: return "alloc_l";
	case set_fp: return "set_fp";
	case add_fp: return "add_fp";
	case nop: return "nop";
	case end: return "end";
	case end_c: return "end_c";
	case save_next: return "save_next";
	case save_reg_any: return "save_reg_any";
	case reserved_custom_stack: return "reserved_custom_stack";
	case reserved2: return "reserved2";
	default: return "Unknown";
	}
}

const char* opcode_id_to_string(pe_bliss::exceptions::arm::unwind_code opcode_id) noexcept
{
	using enum pe_bliss::exceptions::arm::unwind_code;
	switch (opcode_id)
	{
	case alloc_s: return "alloc_s"; break;
	case save_r0r12_lr: return "save_r0r12_lr"; break;
	case mov_sprx: return "mov_sprx"; break;
	case save_r4rx_lr: return "save_r4rx_lr"; break;
	case save_r4rx_lr_wide: return "save_r4rx_lr_wide"; break;
	case save_d8dx: return "save_d8dx"; break;
	case alloc_s_wide: return "alloc_s_wide"; break;
	case save_r0r7_lr: return "save_r0r7_lr"; break;
	case ms_specific: return "ms_specific"; break;
	case ldr_lr_sp: return "ldr_lr_sp"; break;
	case save_dsde: return "save_dsde"; break;
	case save_dsde_16: return "save_dsde_16"; break;
	case alloc_m: return "alloc_m"; break;
	case alloc_m_wide: return "alloc_m_wide"; break;
	case alloc_l: return "alloc_l"; break;
	case alloc_l_wide: return "alloc_l_wide"; break;
	case nop: return "nop"; break;
	case nop_wide: return "nop_wide"; break;
	case end_nop: return "end_nop"; break;
	case end_nop_wide: return "end_nop_wide"; break;
	case end: return "end"; break;
	case reserved: return "reserved"; break;
	default: return "Unknown";
	}
}

template<typename Opcode>
concept opcode_with_offset = requires(Opcode opcode) { opcode.get_offset(); };
template<typename Opcode>
concept opcode_with_thumb2_opcode_bit_size = requires(Opcode) { Opcode::thumb2_opcode_bit_size; };

template<typename Opcode>
void dump_arm_unwind_code(formatter& fmt, const Opcode& opcode)
{
	if constexpr (opcode_with_offset<Opcode>)
	{
		fmt.print_field_name("Offset");
		fmt.get_stream() << ": ";
		fmt.print_value(opcode.get_offset(), true);
		fmt.get_stream() << '\n';
	}
	
	if constexpr (opcode_with_thumb2_opcode_bit_size<Opcode>)
	{
		if constexpr (Opcode::thumb2_opcode_bit_size != 0u)
		{
			fmt.print_field_name("Thumb-2 opcode size (bits)");
			fmt.get_stream() << ": ";
			fmt.print_value(static_cast<std::uint8_t>(Opcode::thumb2_opcode_bit_size), false);
			fmt.get_stream() << '\n';
		}
	}
	
	using namespace pe_bliss::exceptions;

	if constexpr (std::is_same_v<Opcode, arm64::opcode::save_regp>
		|| std::is_same_v<Opcode, arm64::opcode::save_regp_x>)
	{
		fmt.print_field_name("Register pair");
		fmt.get_stream() << ": ";
		fmt.print_string(("x" + std::to_string(opcode.get_register_pair())
			+ ":x" + std::to_string(opcode.get_register_pair() + 1)).c_str());
		fmt.get_stream() << '\n';
	}

	if constexpr (std::is_same_v<Opcode, arm64::opcode::save_reg>
		|| std::is_same_v<Opcode, arm64::opcode::save_reg_x>)
	{
		fmt.print_field_name("Register");
		fmt.get_stream() << ": ";
		fmt.print_string(("x" + std::to_string(opcode.get_register())).c_str());
		fmt.get_stream() << '\n';
	}

	if constexpr (std::is_same_v<Opcode, arm64::opcode::save_lrpair>)
	{
		fmt.print_field_name("Register pair");
		fmt.get_stream() << ": ";
		fmt.print_string(("x" + std::to_string(opcode.get_register())+ ":lr").c_str());
		fmt.get_stream() << '\n';
	}

	if constexpr (std::is_same_v<Opcode, arm64::opcode::save_fregp>
		|| std::is_same_v<Opcode, arm64::opcode::save_fregp_x>)
	{
		fmt.print_field_name("Register pair");
		fmt.get_stream() << ": ";
		fmt.print_string(("d" + std::to_string(opcode.get_register_pair())
			+ ":d" + std::to_string(opcode.get_register_pair() + 1)).c_str());
		fmt.get_stream() << '\n';
	}

	if constexpr (std::is_same_v<Opcode, arm64::opcode::save_freg>
		|| std::is_same_v<Opcode, arm64::opcode::save_freg_x>)
	{
		fmt.print_field_name("Register");
		fmt.get_stream() << ": ";
		fmt.print_string(("d" + std::to_string(opcode.get_register())).c_str());
		fmt.get_stream() << '\n';
	}
}

void dump_arm_unwind_code(formatter& fmt, const opcode_with_delta auto& opcode)
{
	using opcode_type = std::remove_cvref_t<decltype(opcode)>;
	if constexpr (opcode_with_available_check<opcode_type>)
	{
		if (opcode.is_available_opcode())
		{
			fmt.print_string("Undocumented opcode");
			return;
		}
	}
	
	fmt.print_field_name("Delta");
	fmt.get_stream() << ": ";
	fmt.print_value(opcode.get_delta(), true);
	fmt.get_stream() << '\n';
}

void dump_arm_unwind_code(formatter& fmt, const pe_bliss::exceptions::arm::opcode::ms_specific& opcode)
{
	if (opcode.is_available_opcode())
		fmt.print_string("Undocumented opcode");
	else
		fmt.print_string("MS-Specific opcode");
}

const char* register_character_to_string(pe_bliss::exceptions::arm64::opcode::register_character character) noexcept
{
	using enum pe_bliss::exceptions::arm64::opcode::register_character;
	switch (character)
	{
	case x: return "x";
	case d: return "d";
	case q: return "q";
	default: return "Unknown";
	}
}

void dump_arm_unwind_code(formatter& fmt, const pe_bliss::exceptions::arm64::opcode::save_reg_any& opcode)
{
	fmt.print_field_name("Offset");
	fmt.get_stream() << ": ";
	fmt.print_value(opcode.get_offset(), true);
	fmt.get_stream() << " (";
	fmt.print_string(opcode.is_negative_offset() ? "Negative" : "Positive");
	fmt.get_stream() << ')';
	fmt.get_stream() << '\n';

	fmt.print_field_name(opcode.is_reg_pair() ? "Register pair" : "Register");
	fmt.get_stream() << ": ";
	std::string register_text(register_character_to_string(opcode.get_register_character()));
	register_text += std::to_string(opcode.get_register_or_register_pair());
	if (opcode.is_reg_pair())
	{
		register_text += ':';
		register_text += register_character_to_string(opcode.get_register_character());
		register_text += std::to_string(opcode.get_register_or_register_pair() + 1);
	}
	fmt.print_string(register_text.c_str());
	fmt.get_stream() << '\n';
}

const char* custom_stack_case_to_string(pe_bliss::exceptions::arm64::opcode::custom_stack_case value) noexcept
{
	using enum pe_bliss::exceptions::arm64::opcode::custom_stack_case;
	switch (value)
	{
	case msft_op_trap_frame: return "MSFT_OP_TRAP_FRAME";
	case msft_op_machine_frame: return "MSFT_OP_MACHINE_FRAME";
	case msft_op_context: return "MSFT_OP_CONTEXT";
	case msft_op_clear_unwound_to_call: return "MSFT_OP_CLEAR_UNWOUND_TO_CALL";
	default: return "Unknown";
	}
}

void dump_arm_unwind_code(formatter& fmt,
	const pe_bliss::exceptions::arm64::opcode::reserved_custom_stack& opcode)
{
	fmt.print_field_name("Custom stack case");
	fmt.get_stream() << ": ";
	fmt.print_string(custom_stack_case_to_string(opcode.get_custom_stack_case()));
	fmt.get_stream() << '\n';
}

void dump_arm_unwind_code(formatter& fmt, const opcode_with_allocation_size auto& opcode)
{
	dump_unwind_code(fmt, opcode);
}

void dump_arm_unwind_code(formatter& fmt, const opcode_with_saved_int_registers auto& opcode)
{
	fmt.print_field_name("Saved registers");
	fmt.get_stream() << ": ";

	if (static_cast<std::uint32_t>(opcode.get_saved_registers()) == 0u)
	{
		fmt.print_string("None");
	}
	else
	{
		using enum pe_bliss::exceptions::arm::opcode::int_registers::value;
		fmt.print_comma_separated_flags(opcode.get_saved_registers(), {
			{ r0, "r0" },
			{ r1, "r1" },
			{ r2, "r2" },
			{ r3, "r3" },
			{ r4, "r4" },
			{ r5, "r5" },
			{ r6, "r6" },
			{ r7, "r7" },
			{ r8, "r8" },
			{ r9, "r9" },
			{ r10, "r10" },
			{ r11, "r11" },
			{ r12, "r12" },
			{ lr, "lr" }
		});
	}

	fmt.get_stream() << '\n';
}

void dump_arm_unwind_code(formatter& fmt, const opcode_with_saved_fp_registers auto& opcode)
{
	fmt.print_field_name("Saved registers");
	fmt.get_stream() << ": ";

	if (static_cast<std::uint32_t>(opcode.get_saved_registers()) == 0u)
	{
		fmt.print_string("None");
	}
	else
	{
		using enum pe_bliss::exceptions::arm::opcode::fp_registers::value;
		fmt.print_comma_separated_flags(opcode.get_saved_registers(), {
			{ d0, "d0" },
			{ d1, "d1" },
			{ d2, "d2" },
			{ d3, "d3" },
			{ d4, "d4" },
			{ d5, "d5" },
			{ d6, "d6" },
			{ d7, "d7" },
			{ d8, "d8" },
			{ d9, "d9" },
			{ d10, "d10" },
			{ d11, "d11" },
			{ d12, "d12" },
			{ d13, "d13" },
			{ d14, "d14" },
			{ d15, "d15" }
		});
	}

	fmt.get_stream() << '\n';
}

void dump_arm_unwind_code(formatter& fmt, const opcode_with_registers_range auto& opcode)
{
	auto range = opcode.get_saved_registers_range();
	fmt.print_field_name("Saved registers");
	fmt.get_stream() << ": ";
	fmt.print_string("d");
	fmt.print_string(std::to_string(static_cast<std::uint8_t>(range.first)).c_str());
	fmt.print_string("-");
	fmt.print_string("d");
	fmt.print_string(std::to_string(static_cast<std::uint8_t>(range.second)).c_str());
	fmt.get_stream() << '\n';
}

template<typename EpilogInfo, typename UnwindRecordOptions>
void dump_unwind_info(formatter& fmt,
	const pe_bliss::exceptions::arm_common::extended_unwind_record<EpilogInfo, UnwindRecordOptions>& data)
{
	fmt.print_structure_name("Extended unwind record");
	fmt.get_stream() << '\n';

	fmt.print_field_name("Main header");
	fmt.print_offsets_and_value(data.get_main_header(), true);
	fmt.get_stream() << '\n';

	if (data.has_extended_main_header())
	{
		fmt.print_field_name("Main extended header");
		fmt.print_offsets_and_value(data.get_main_extended_header(), true);
		fmt.get_stream() << '\n';
	}

	if (data.has_exception_data())
	{
		fmt.print_field_name("Exception handler RVA");
		fmt.print_offsets_and_value(data.get_exception_handler_rva(), true);
		fmt.get_stream() << '\n';
	}

	fmt.print_field_name("Struct version");
	fmt.get_stream() << ": ";
	fmt.print_value(data.get_version(), true);
	fmt.get_stream() << '\n';

	fmt.print_field_name("Function length");
	fmt.get_stream() << ": ";
	fmt.print_value(data.get_function_length(), true);
	fmt.get_stream() << '\n';

	fmt.print_field_name(data.single_epilog_info_packed()
		? "Single epilog first unwind code index" : "Epilog scope count");
	fmt.get_stream() << ": ";
	fmt.print_value(data.get_epilog_count(), true);
	fmt.get_stream() << '\n';

	fmt.print_field_name("Code words count");
	fmt.get_stream() << ": ";
	fmt.print_value(data.get_code_words(), true);
	fmt.get_stream() << '\n';

	if constexpr (UnwindRecordOptions::has_f_bit)
	{
		fmt.print_field_name("Is function fragment");
		fmt.get_stream() << ": ";
		fmt.print_string(data.is_function_fragment() ? "YES" : "NO");
		fmt.get_stream() << '\n';
	}

	if (!data.single_epilog_info_packed() && !data.get_epilog_info_list().empty())
	{
		fmt.get_stream() << '\n';
		fmt.print_structure_name("Epilog scopes");
		fmt.get_stream() << '\n';
		for (const auto& epilog : data.get_epilog_info_list())
		{
			fmt.print_field_name("Descriptor");
			fmt.print_offsets_and_value(epilog.get_descriptor(), true);
			fmt.get_stream() << "; ";

			fmt.print_field_name("Start index");
			fmt.get_stream() << ": ";
			fmt.print_value(epilog.get_epilog_start_index(), true);
			fmt.get_stream() << "; ";

			fmt.print_field_name("Start offset");
			fmt.get_stream() << ": ";
			fmt.print_value(epilog.get_epilog_start_offset(), true);
			fmt.get_stream() << '\n';
		}
	}

	if (!data.get_unwind_code_list().empty())
	{
		fmt.get_stream() << '\n';
		fmt.print_structure_name("Unwind codes");
		fmt.get_stream() << '\n';

		for (const auto& code : data.get_unwind_code_list())
		{
			std::visit([&fmt] (const auto& code) {
				fmt.print_field_name("Code");
				fmt.get_stream() << ": ";
				fmt.print_string(opcode_id_to_string(code.opcode));
				fmt.get_stream() << '\n';

				fmt.print_field_name("Descriptor");
				fmt.print_bytes(nullptr, code.get_descriptor());

				dump_arm_unwind_code(fmt, code);
				fmt.get_stream() << '\n';
			}, code);
		}
	}

	fmt.get_stream() << '\n';
}

template<typename ArmRuntimeFunction>
void dump_runtime_function(formatter& fmt, const ArmRuntimeFunction& func)
{
	fmt.print_structure("Runtime function descriptor", func.get_descriptor(), std::array{
		value_info{"begin_address"},
		value_info{"unwind_info"}
	});

	fmt.print_errors(func);

	std::visit([&fmt] (const auto& info) {
		dump_unwind_info(fmt, info);
	}, func.get_unwind_info());
}

template<typename Directory>
struct directory_name {};
template<>
struct directory_name<pe_bliss::exceptions::arm::exception_directory_details>
{
	static constexpr const char* name = "ARM exception directory";
};
template<>
struct directory_name<pe_bliss::exceptions::arm64::exception_directory_details>
{
	static constexpr const char* name = "ARM64 exception directory";
};
template<>
struct directory_name<pe_bliss::exceptions::x64::exception_directory_details>
{
	static constexpr const char* name = "X64 exception directory";
};

template<typename Directory>
void dump_directory(formatter& fmt, const Directory& directory)
{
	fmt.get_stream() << "===== ";
	fmt.print_structure_name(directory_name<Directory>::name);
	fmt.get_stream() << " =====\n\n";
	fmt.print_errors(directory);

	for (const auto& func : directory.get_runtime_function_list())
		dump_runtime_function(fmt, func);
}

} //namespace

void dump_exceptions(formatter& fmt, const pe_bliss::image::image& image) try
{
	auto exceptions = pe_bliss::exceptions::load(image, {});
	if (exceptions.get_directories().empty())
		return;

	fmt.get_stream() << "===== ";
	fmt.print_structure_name("Exception directories");
	fmt.get_stream() << " =====\n\n";
	fmt.print_errors(exceptions);

	for (const auto& directory : exceptions.get_directories())
		std::visit([&fmt] (const auto& dir) { dump_directory(fmt, dir); }, directory);
}
catch (const std::system_error& e)
{
	fmt.print_error("Error loading exceptions:", e);
}
catch (const std::exception& e)
{
	fmt.print_error("Error loading exceptions:", e);
}
