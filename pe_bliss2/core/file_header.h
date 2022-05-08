#pragma once

#include <cstddef>
#include <cstdint>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/detail/image_file_header.h"
#include "pe_bliss2/detail/packed_struct_base.h"
#include "pe_bliss2/pe_error.h"

#include "utilities/static_class.h"

namespace buffers
{
class input_buffer_interface;
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::core
{

enum class file_header_errc
{
	unable_to_read_file_header = 1
};

std::error_code make_error_code(file_header_errc) noexcept;

class [[nodiscard]] file_header
	: public detail::packed_struct_base<detail::image_file_header>
{
public:
	enum class machine_type : std::uint16_t
	{
		target_host = 0x0001, //Useful for indicating we want to interact with the host and not a WoW guest.
		i386 = 0x14c, //Intel 386 machine or later and compatible processors
		i486 = 0x14d, //Intel 486 machine
		pentium = 0x14e, //Intel Pentium machine
		r3000_mips = 0x162, //MIPS little-endian, 0540 big-endian machine
		r4000_mips = 0x166, //MIPS little-endian machine
		r10000_mips = 0x168, //MIPS little-endian machine
		mips_wce_v2 = 0x169, //MIPS little-endian WCE v2 machine
		alpha_axp = 0x184, //Alpha AXP machine
		sh3 = 0x1a2, //Hitachi SH3 little-endian machine
		sh3dsp = 0x1a3, //Hitachi SH3DSP machine
		sh3e = 0x1a4, //Hitachi SH3E little-endian machine
		sh4 = 0x1a6, //Hitachi SH4 little-endian machine
		sh5 = 0x1a8, //Hitachi SH5 little-endian machine
		arm = 0x1c0, //ARM little endian machine
		arm_thumb_or_v7 = 0x1c2, //ARM v7 little-endian machine
		armnt = 0x01c4, //ARM Thumb-2 Little-Endian
		arm_am33 = 0x1d3, //Matsushita AM33 machine
		ibm_power_pc = 0x1f0, //Power PC little-endian machine
		ibm_power_pc_fp = 0x1f1, //Power PC FP little-endian machine
		ibm_power_pc_be = 0x1f2, //Power PC big-endian machine
		ia64 = 0x200, //Intel IA64 machine
		mips16 = 0x266, //MIPS16 machine
		mips_fpu = 0x366, //MIPS with FPU machine
		mips_fpu16 = 0x466, //MIPS16 with FPU machine
		aplha64 = 0x284, //Alpha AXP 64-bit machine
		infineon_tricore = 0x520, //Infineon TRICORE machine
		infenion_cef = 0xcef, //Infineon CEF machine
		efi_byte_code = 0xebc, //Machine that supports EFI byte code
		amd64 = 0x8664, //AMD64 (K8) 64-bit machine
		m32r = 0x9041, //Mitsubishi M32R little-endian machine
		cee = 0xc0ee, //CEE machine
		sparc = 0x2000, //SPARC machine
		dec_alpha_axp = 0x183, //DEC Alpha AXP machine
		m68k = 0x268, //Motorola 68000 series machine
		tahoe = 0x7cc, //Intel EM machine,
		arm64 = 0xAA64, //ARM64 Little-Endian
		chpe_x86 = 0x3a64 //Both x86 and ARM64 code in single binary

		//chpe_x86 - Both x86 and ARM64 code in single binary
		//arm64 + hybrid_metadata_pointer = CHPEv2 - both x64 and Arm64 in single binary
		//amd64 + hybrid_metadata_pointer = CHPEv2 - both x64 and Arm64 in single binary
	};

	struct characteristics final : utilities::static_class
	{
		enum value : std::uint16_t
		{
			relocs_stripped = 0x0001, // Relocation info stripped from file.
			executable_image = 0x0002, // File is executable  (i.e. no unresolved externel references).
			line_nums_stripped = 0x0004, // Line numbers stripped from file.
			local_syms_stripped = 0x0008, // Local symbols stripped from file.
			aggresive_ws_trim = 0x0010, // Agressively trim working set
			large_address_aware = 0x0020, // App can handle >2gb addresses
			bytes_reversed_lo = 0x0080, // Bytes of machine word are reversed.
			machine_32bit = 0x0100, // 32 bit word machine.
			debug_stripped = 0x0200, // Debugging info stripped from file in .DBG file
			removable_run_from_swap = 0x0400, // If Image is on removable media, copy and run from the swap file.
			net_run_from_swap = 0x0800, // If Image is on Net, copy and run from the swap file.
			system = 0x1000, // System File.
			dll = 0x2000, // File is a DLL.
			up_system_only = 0x4000, // File should only be run on a UP machine
			bytes_reversed_hi = 0x8000 // Bytes of machine word are reversed.
		};
	};

public:
	//When deserializing, buffer should point to the file header
	//(next field after image signature)
	void deserialize(buffers::input_buffer_interface& buf,
		bool allow_virtual_memory = false);
	void serialize(buffers::output_buffer_interface& buf,
		bool write_virtual_part = true) const;

public:
	[[nodiscard]]
	machine_type get_machine_type() const noexcept
	{
		return static_cast<machine_type>(base_struct()->machine);
	}

	[[nodiscard]]
	std::size_t get_section_table_buffer_pos() const noexcept;

	[[nodiscard]]
	characteristics::value get_characteristics() const noexcept
	{
		return static_cast<characteristics::value>(
			base_struct()->characteristics);
	}

	void set_machine_type(machine_type type) noexcept
	{
		base_struct()->machine = static_cast<std::uint16_t>(type);
	}

	void set_characteristics(characteristics::value value) noexcept
	{
		base_struct()->characteristics = value;
	}

	[[nodiscard]] bool is_dll() const noexcept
	{
		return static_cast<bool>(get_characteristics() & characteristics::dll);
	}
};

} //namespace pe_bliss::core

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::core::file_header_errc> : true_type {};
} //namespace std
