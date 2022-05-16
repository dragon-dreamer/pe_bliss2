#include "rich_data_dumper.h"

#include <system_error>

#include "formatter.h"

#include "pe_bliss2/image.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/rich/compid_database.h"
#include "pe_bliss2/rich/rich_header.h"
#include "pe_bliss2/rich/rich_header_loader.h"

void dump_rich_data(formatter& fmt, const pe_bliss::image& image) try
{
	auto result = pe_bliss::rich::load(*image.get_dos_stub().data());
	if (!result)
		return;

	fmt.print_structure_name("Rich header");
	const auto& header = *result;
	fmt.get_stream() << ' ';
	fmt.print_absolute_offset(header.get_absolute_offset());
	fmt.get_stream() << "\n  ";
	fmt.print_field_name("Checksum");
	fmt.get_stream() << ' ';
	fmt.print_value(header.get_checksum(), true);
	fmt.get_stream() << "\n  ";
	fmt.print_field_name("Calculated checksum");
	fmt.get_stream() << ' ';
	fmt.print_value(header.calculate_checksum(*image.get_full_headers_buffer().data()),
		true);
	fmt.get_stream() << '\n';

	const auto& compids = header.get_compids();
	for (const auto& compid : compids)
	{
		fmt.get_stream() << "  ";
		fmt.print_string(pe_bliss::rich::compid_database::tool_type_to_string(
			pe_bliss::rich::compid_database::get_tool(compid.prod_id)));

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

		auto product_info = pe_bliss::rich::compid_database::get_product(compid);
		fmt.get_stream() << ", ";
		fmt.print_string(pe_bliss::rich::compid_database::product_type_to_string(product_info.type));
		fmt.get_stream() << " (exact match: ";
		fmt.print_string(product_info.exact ? "YES" : "NO");
		fmt.get_stream() << ')';
		fmt.get_stream() << '\n';
	}

	fmt.get_stream() << '\n';
}
catch (const std::system_error& e)
{
	fmt.print_error("Error loading Rich header:", e);
}
catch (const std::exception& e)
{
	fmt.print_error("Error loading Rich header:", e);
}
