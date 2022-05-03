#include "dos_stub_dumper.h"

#include <array>

#include "color_provider.h"
#include "formatter.h"

#include "pe_bliss2/dos_stub.h"

void dump_dos_stub(formatter& fmt, const pe_bliss::dos_stub& stub)
{
	fmt.print_structure_name("DOS stub");
	fmt.get_stream() << ' ';
	fmt.print_absolute_offset(stub.data()->absolute_offset());
	fmt.get_stream() << ", size ";
	{
		color_changer changer(fmt.get_stream(), fmt.get_color_provider(),
			fmt.value_fg_color, fmt.value_bg_color);
		fmt.get_stream() << std::dec << stub.data()->size();
	}
	fmt.get_stream() << "\n\n";
}
