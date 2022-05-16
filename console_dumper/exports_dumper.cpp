#include "exports_dumper.h"

#include <array>
#include <exception>
#include <functional>
#include <system_error>

#include "formatter.h"

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/exports/export_directory.h"
#include "pe_bliss2/exports/export_directory_builder.h"
#include "pe_bliss2/exports/export_directory_loader.h"

void dump_exports(formatter& fmt, const pe_bliss::image::image& image) try
{
	auto exports = pe_bliss::exports::load(image, {});
	if (!exports)
		return;

	fmt.get_stream() << "===== ";
	fmt.print_structure_name("Export directory");
	fmt.get_stream() << " =====\n\n";
	fmt.print_errors(*exports);

	fmt.print_structure("Export descriptor", exports->get_descriptor(), std::array{
		value_info{"characteristics"},
		value_info{"time_date_stamp"},
		value_info{"major_version"},
		value_info{"minor_version"},
		value_info{"name", true, std::bind(&formatter::print_packed_string, std::ref(fmt),
			std::cref(exports->get_library_name()))},
		value_info{"base"},
		value_info{"number_of_functions"},
		value_info{"number_of_names"},
		value_info{"address_of_functions"},
		value_info{"address_of_names"},
		value_info{"address_of_name_ordinals"}
	});

	fmt.print_structure_name("Export list");
	fmt.get_stream() << '\n';
	if (exports->get_export_list().empty())
		fmt.get_stream() << "No exports";

	for (const auto& address : exports->get_export_list())
	{
		fmt.print_field_name("RVA");
		fmt.get_stream() << ' ';
		fmt.print_offsets_and_value(address.get_rva(), true);

		fmt.get_stream() << ' ';
		fmt.print_field_name("RVA ordinal");
		fmt.get_stream() << ' ';
		fmt.print_value(address.get_rva_ordinal(), true);

		if (address.get_forwarded_name())
		{
			fmt.get_stream() << ' ';
			fmt.print_field_name("Forwarded to ");
			fmt.print_packed_string(*address.get_forwarded_name());
		}

		fmt.get_stream() << '\n';
		fmt.print_errors(address);

		for (const auto& name : address.get_names())
		{
			fmt.get_stream() << "    ";
			if (name.get_name())
			{
				fmt.print_field_name("Name RVA");
				fmt.get_stream() << ' ';
				fmt.print_offsets_and_value(name.get_name_rva(), true);
				fmt.get_stream() << ' ';
				fmt.print_field_name("Name");
				fmt.get_stream() << ' ';
				fmt.print_packed_string(*name.get_name());
				fmt.get_stream() << ' ';
			}
			fmt.print_field_name("Name ordinal");
			fmt.get_stream() << ' ';
			fmt.print_offsets_and_value(name.get_name_ordinal(), true);
			fmt.get_stream() << '\n';
			fmt.print_errors(address);
		}
	}

	fmt.get_stream() << '\n';
}
catch (const std::system_error& e)
{
	fmt.print_error("Error loading exports:", e);
}
catch (const std::exception& e)
{
	fmt.print_error("Error loading exports:", e);
}
