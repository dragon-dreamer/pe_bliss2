#include "optional_header_dumper.h"

#include <array>
#include <cstddef>
#include <functional>
#include <variant>

#include "formatter.h"

#include "pe_bliss2/optional_header.h"

namespace
{

const char* subsystem_to_string(pe_bliss::optional_header::subsystem value)
{
	using enum pe_bliss::optional_header::subsystem;
	switch (value)
	{
	case native: return "Native";
	case windows_gui: return "Windows GUI";
	case windows_cui: return "Windows CUI";
	case os2_cui: return "OS2 CUI";
	case posix_cui: return "Posix CUI";
	case windows_ce_gui: return "Windows CE GUI";
	case efi_application: return "EFI Application";
	case efi_boot_service_driver: return "EFI Boot Service Driver";
	case efi_runtime_driver: return "EFI Runtime Driver";
	case efi_rom: return "EFI ROM";
	case xbox: return "XBOX";
	case windows_boot_application: return "Windows Boot Application";
	case xbox_code_catalog: return "XBOX Code Catalog";
	default: return "Unknown";
	}
}

void dump_optional_header_subsystem(formatter& fmt, const pe_bliss::optional_header& header)
{
	fmt.get_stream() << '(';
	fmt.print_string(subsystem_to_string(header.get_subsystem()));
	fmt.get_stream() << ')';
}

void dump_optional_header_dll_characteristics(formatter& fmt,
	const pe_bliss::optional_header& header, std::size_t left_padding)
{
	using enum pe_bliss::optional_header::dll_characteristics::value;
	fmt.print_flags(header.get_dll_characteristics(), left_padding, {
		{ high_entropy_va, "HIGH_ENTROPY_VA" },
		{ dynamic_base, "DYNAMIC_BASE" },
		{ force_integrity, "FORCE_INTEGRITY" },
		{ nx_compat, "NX_COMPAT" },
		{ no_isolation, "NO_ISOLATION" },
		{ no_seh, "NO_SEH" },
		{ no_bind, "NO_BIND" },
		{ appcontainer, "APPCONTAINER" },
		{ wdm_driver, "WDM_DRIVER" },
		{ guard_cf, "GUARD_CF" },
		{ terminal_server_aware, "TERMINAL_SERVER_AWARE" }
	});
}

template<typename Header>
void dump_optional_header(formatter& fmt,
	const pe_bliss::optional_header& header, const Header& value)
{
	fmt.print_structure_name("Magic");
	fmt.get_stream() << ' ';
	fmt.print_absolute_offset(value.get_state().absolute_offset()
		- sizeof(pe_bliss::optional_header::magic_type));
	fmt.get_stream() << " (";
	fmt.print_string(header.get_magic() == pe_bliss::optional_header::magic::pe32
		? "PE" : "PE+");
	fmt.get_stream() << ")\n\n";

	fmt.print_structure("Optional header", value, std::array{
		value_info{"major_linker_version", false},
		value_info{"minor_linker_version", false},
		value_info{"size_of_code"},
		value_info{"size_of_initialized_data"},
		value_info{"size_of_uninitialized_data"},
		value_info{"address_of_entry_point"},
		value_info{"base_of_code"},
		value_info{header.get_magic() == pe_bliss::optional_header::magic::pe32
			? "base_of_data" : nullptr},
		value_info{"image_base"},
		value_info{"section_alignment"},
		value_info{"file_alignment"},
		value_info{"major_operating_system_version", false},
		value_info{"minor_operating_system_version", false},
		value_info{"major_image_version", false},
		value_info{"minor_image_version", false},
		value_info{"major_subsystem_version", false},
		value_info{"minor_subsystem_version", false},
		value_info{"win32_version_value", false},
		value_info{"size_of_image"},
		value_info{"size_of_headers"},
		value_info{"checksum"},
		value_info{"subsystem", true,
			std::bind(dump_optional_header_subsystem, std::ref(fmt), std::cref(header))},
		value_info{"dll_characteristics", true,
			std::bind(dump_optional_header_dll_characteristics, std::ref(fmt),
				std::cref(header), std::placeholders::_1)},
		value_info{"size_of_stack_reserve"},
		value_info{"size_of_stack_commit"},
		value_info{"size_of_heap_reserve"},
		value_info{"size_of_heap_commit"},
		value_info{"loader_flags"},
		value_info{"number_of_rva_and_sizes"}
	});
}

} //namespace

void dump_optional_header(formatter& fmt, const pe_bliss::optional_header& header)
{
	std::visit([&fmt, &header] (const auto& value) {
		dump_optional_header(fmt, header, value);
	}, header.base_struct());
}
