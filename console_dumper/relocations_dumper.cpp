#include "relocations_dumper.h"

#include <array>
#include <exception>
#include <system_error>

#include "formatter.h"

#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/relocations/relocation_entry.h"
#include "pe_bliss2/relocations/relocation_directory_loader.h"
#include "pe_bliss2/relocations/relocation_entry.h"

namespace
{
const char* relocation_type_to_string(pe_bliss::core::file_header::machine_type machine,
	pe_bliss::relocations::relocation_type type)
{
	using enum pe_bliss::relocations::relocation_type;
	switch (type)
	{
	case absolute: return "ABSOLUTE";
	case high: return "HIGH";
	case low: return "LOW";
	case highlow: return "HIGHLOW";
	case highadj: return "HIGHADJ";
	case mips_jmpaddr: return "MIPS_JMPADDR/MIPS_JMPADDR/RISCV_HIGH20";
	case thumb_mov32:
		return machine == pe_bliss::core::file_header::machine_type::armnt
			? "THUMB_MOV32" : "RISCV_LOW12I";
	case riscv_low12s: return "RISCV_LOW12S";
	case mips_jmpaddr16: return "MIPS_JMPADDR16";
	case dir64: return "DIR64";
	case high3adj: return "HIGH3ADJ";
	default: return "UNKNOWN";
	}
}
} //namespace

void dump_relocations(formatter& fmt, const pe_bliss::image::image& image) try
{
	auto relocations = pe_bliss::relocations::load(image, {});
	if (!relocations)
		return;

	fmt.get_stream() << "===== ";
	fmt.print_structure_name("Relocation directory");
	fmt.get_stream() << " =====\n\n";
	fmt.print_errors(relocations->errors);

	auto machine = image.get_file_header().get_machine_type();
	for (const auto& table : relocations->relocations)
	{
		fmt.print_structure("Relocation table descriptor", table.get_descriptor(), std::array{
			value_info{"virtual_address"},
			value_info{"size_of_block"}
		});

		fmt.print_errors(table);

		fmt.print_structure_name("Relocation list");
		fmt.get_stream() << '\n';
		if (table.get_relocations().empty())
			fmt.get_stream() << "No relocations";

		for (const auto& reloc : table.get_relocations())
		{
			fmt.print_field_name("Descriptor");
			fmt.print_offsets_and_value(reloc.get_descriptor(), true);

			fmt.get_stream() << ' ';
			fmt.print_field_name("RVA");
			fmt.get_stream() << ' ';
			fmt.print_value(reloc.get_address() + table.get_descriptor()->virtual_address, true);

			try
			{
				auto size = reloc.get_affected_size_in_bytes(machine);
				fmt.get_stream() << ' ';
				fmt.print_field_name("Size");
				fmt.get_stream() << ' ';
				fmt.print_value(size, true);
			}
			catch (const std::system_error&)
			{
			}

			if (reloc.get_param())
			{
				fmt.get_stream() << ' ';
				fmt.print_field_name("Param");
				fmt.print_offsets_and_value(reloc.get_descriptor(), true);
			}

			fmt.get_stream() << " (";
			fmt.print_string(relocation_type_to_string(machine, reloc.get_type()));
			fmt.get_stream() << ")\n";

			fmt.print_errors(reloc);
		}

		fmt.get_stream() << '\n';
	}
}
catch (const std::system_error& e)
{
	fmt.print_error("Error loading relocations:", e);
}
catch (const std::exception& e)
{
	fmt.print_error("Error loading relocations:", e);
}
