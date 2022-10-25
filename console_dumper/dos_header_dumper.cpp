#include "dos_header_dumper.h"

#include <array>

#include "formatter.h"

#include "pe_bliss2/dos/dos_header.h"

void dump_dos_header(formatter& fmt, const pe_bliss::dos::dos_header& header)
{
	fmt.print_structure("DOS header", header.get_descriptor(), std::array{
		value_info{"e_magic"},
		value_info{"e_cblp"},
		value_info{"e_cp"},
		value_info{"e_crlc"},
		value_info{"e_cparhdr"},
		value_info{"e_minalloc"},
		value_info{"e_maxalloc"},
		value_info{"e_ss"},
		value_info{"e_sp"},
		value_info{"e_csum"},
		value_info{"e_ip"},
		value_info{"e_cs"},
		value_info{"e_lfarlc"},
		value_info{"e_ovno"},
		value_info{"e_res"},
		value_info{"e_oemid"},
		value_info{"e_oeminfo"},
		value_info{"e_res2"},
		value_info{"e_lfanew"}
	});
}
