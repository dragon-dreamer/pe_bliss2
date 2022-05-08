#include "file_header_dumper.h"

#include <array>
#include <cstddef>
#include <functional>

#include "formatter.h"

#include "pe_bliss2/core/file_header.h"

namespace
{

void dump_file_header_characteristics(formatter& fmt,
	const pe_bliss::core::file_header& header, std::size_t left_padding)
{
	using enum pe_bliss::core::file_header::characteristics::value;
	fmt.print_flags(header.get_characteristics(), left_padding, {
		{ relocs_stripped, "RELOCS_STRIPPED" },
		{ executable_image, "EXECUTABLE_IMAGE" },
		{ line_nums_stripped, "LINE_NUMS_STRIPPED" },
		{ local_syms_stripped, "LOCAL_SYMS_STRIPPED" },
		{ aggresive_ws_trim, "AGGRESIVE_WS_TRIM" },
		{ large_address_aware, "LARGE_ADDRESS_AWARE" },
		{ bytes_reversed_lo, "BYTES_REVERSED_LO" },
		{ machine_32bit, "MACHINE_32BIT" },
		{ debug_stripped, "DEBUG_STRIPPED" },
		{ removable_run_from_swap, "REMOVABLE_RUN_FROM_SWAP" },
		{ net_run_from_swap, "NET_RUN_FROM_SWAP" },
		{ system, "SYSTEM" },
		{ dll, "DLL" },
		{ up_system_only, "UP_SYSTEM_ONLY" },
		{ bytes_reversed_hi, "BYTES_REVERSED_HI" },
	});
}

const char* machine_to_string(pe_bliss::core::file_header::machine_type machine)
{
	using enum pe_bliss::core::file_header::machine_type;
	switch (machine)
	{
	case target_host: return "TARGET_HOST";
	case i386: return "I386";
	case i486: return "I486";
	case pentium: return "PENTIUM";
	case r3000_mips: return "R3000_MIPS";
	case r4000_mips: return "R4000_MIPS";
	case r10000_mips: return "R10000_MIPS";
	case mips_wce_v2: return "MIPS_WCE_V2";
	case alpha_axp: return "ALPHA_AXP";
	case sh3: return "SH3";
	case sh3dsp: return "SH3DSP";
	case sh3e: return "SH3E";
	case sh4: return "SH4";
	case sh5: return "SH5";
	case arm: return "ARM";
	case arm_thumb_or_v7: return "ARM_THUMB_OR_V7";
	case armnt: return "ARMNT";
	case arm_am33: return "ARM_AM33";
	case ibm_power_pc: return "IBM_POWER_PC";
	case ibm_power_pc_fp: return "IBM_POWER_PC_FP";
	case ibm_power_pc_be: return "IBM_POWER_PC_BE";
	case ia64: return "IA64";
	case mips16: return "MIPS16";
	case mips_fpu: return "MIPS_FPU";
	case mips_fpu16: return "MIPS_FPU16";
	case aplha64: return "APLHA64";
	case infineon_tricore: return "INFINEON_TRICORE";
	case infenion_cef: return "INFENION_CEF";
	case efi_byte_code: return "EFI_BYTE_CODE";
	case amd64: return "AMD64";
	case m32r: return "M32R";
	case cee: return "CEE";
	case sparc: return "SPARC";
	case dec_alpha_axp: return "DEC_ALPHA_AXP";
	case m68k: return "M68K";
	case tahoe: return "TAHOE";
	case arm64: return "ARM64";
	case chpe_x86: return "CHPE_X86";
	default: return "Unknown";
	}
}

void dump_file_header_machine(formatter& fmt, const pe_bliss::core::file_header& header)
{
	fmt.get_stream() << '(';
	fmt.print_string(machine_to_string(header.get_machine_type()));
	fmt.get_stream() << ')';
}

} //namespace

void dump_file_header(formatter& fmt, const pe_bliss::core::file_header& header)
{
	fmt.print_structure("File header", header.base_struct(), std::array{
		value_info{"machine", true, std::bind(
			dump_file_header_machine, std::ref(fmt), std::cref(header))},
		value_info{"number_of_sections"},
		value_info{"time_date_stamp"},
		value_info{"pointer_to_symbol_table"},
		value_info{"number_of_symbols"},
		value_info{"size_of_optional_header"},
		value_info{"characteristics", true, std::bind(dump_file_header_characteristics,
			std::ref(fmt), std::cref(header), std::placeholders::_1)}
	});
}
