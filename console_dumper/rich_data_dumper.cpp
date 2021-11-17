#include "rich_data_dumper.h"

#include "formatter.h"

#include "pe_bliss2/compid_database.h"
#include "pe_bliss2/dos_stub.h"
#include "pe_bliss2/rich_header.h"

void dump_rich_data(formatter& fmt, const pe_bliss::dos_stub& stub,
	const pe_bliss::rich_header& header)
{
	if (!header.is_valid())
		return;

	fmt.print_structure_name("Rich header");
	fmt.get_stream() << ' ';
	fmt.print_absolute_offset(stub.buffer_pos() + header.dos_stub_offset());
	fmt.get_stream() << "\n  ";
	fmt.print_field_name("Checksum");
	fmt.get_stream() << ' ';
	fmt.print_value(header.get_checksum(), true);
	fmt.get_stream() << '\n';

	const auto& compids = header.get_compids();
	for (const auto& compid : compids)
	{
		fmt.get_stream() << "  ";
		fmt.print_string(pe_bliss::compid_database::tool_type_to_string(
			pe_bliss::compid_database::get_tool(compid.prod_id)));

		fmt.get_stream() << ", ";
		fmt.print_field_name("PRODID");
		fmt.get_stream() << ' ';
		fmt.print_value(compid.prod_id, true);
		fmt.get_stream() << ", ";
		fmt.print_field_name("BUILD");
		fmt.get_stream() << ' ';
		fmt.print_value(compid.build_number, true);
		fmt.get_stream() << ", ";
		fmt.print_field_name("USE COUNT");
		fmt.get_stream() << ' ';
		fmt.print_value(compid.use_count, false);

		auto product_info = pe_bliss::compid_database::get_product(compid);
		fmt.get_stream() << ", ";
		fmt.print_string(pe_bliss::compid_database::product_type_to_string(product_info.type));
		fmt.get_stream() << " (exact match: ";
		fmt.print_string(product_info.exact ? "YES" : "NO");
		fmt.get_stream() << ')';
		fmt.get_stream() << '\n';
	}

	fmt.get_stream() << '\n';
}
