#include "imports_dumper.h"

#include <array>
#include <exception>
#include <functional>
#include <system_error>
#include <variant>
#include <vector>

#include "formatter.h"

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/imports/imported_address.h"
#include "pe_bliss2/imports/import_directory.h"
#include "pe_bliss2/imports/import_directory_loader.h"

namespace
{

void dump_is_library_bound(formatter& fmt, bool is_bound)
{
	if (!is_bound)
		return;

	fmt.get_stream() << '(';
	fmt.print_string("Bound");
	fmt.get_stream() << ')';
}

template<typename Va>
void dump_import_info(formatter& fmt,
	const pe_bliss::imports::imported_function_address<Va>& address)
{
	if (!address.get_imported_va())
		return;

	fmt.get_stream() << ' ';
	fmt.print_field_name("FuncVA");
	fmt.get_stream() << ' ';
	fmt.print_offsets_and_value(*address.get_imported_va(), true);
}

template<typename Va>
void dump_import_info(formatter& fmt,
	const pe_bliss::imports::imported_function_ordinal<Va>& ordinal)
{
	fmt.print_field_name("Ordinal");
	fmt.get_stream() << ' ';
	fmt.print_value(ordinal.get_ordinal(), true);

	dump_import_info(fmt,
		static_cast<const pe_bliss::imports::imported_function_address<Va>&>(ordinal));
}

template<typename Va>
void dump_import_info(formatter& fmt,
	const pe_bliss::imports::imported_function_hint_and_name<Va>& hint_name)
{
	fmt.print_field_name("Hint");
	fmt.print_offsets_and_value(hint_name.get_hint(), true);
	fmt.get_stream() << ' ';
	fmt.print_field_name("Name");
	fmt.print_packed_string(hint_name.get_name());
	dump_import_info(fmt,
		static_cast<const pe_bliss::imports::imported_function_address<Va>&>(hint_name));
}

template<typename Va, typename Descriptor>
void dump_imports(formatter& fmt,
	const std::vector<pe_bliss::imports::imported_library_details<Va, Descriptor>>& imports) try
{
	for (const auto& lib : imports)
	{
		fmt.print_errors(lib);

		fmt.print_structure("Imported library descriptor", lib.get_descriptor(), std::array{
			value_info{"lookup_table"},
			value_info{"time_date_stamp", true,
				std::bind(dump_is_library_bound, std::ref(fmt), lib.is_bound())},
			value_info{"forwarder_chain"},
			value_info{"name", true, std::bind(
				&formatter::print_packed_string<pe_bliss::packed_c_string>, std::ref(fmt),
				std::cref(lib.get_library_name()))},
			value_info{"address_table"}
		});

		for (const auto& import : lib.get_imports())
		{
			if (import.get_lookup())
			{
				fmt.print_field_name("ILT");
				fmt.print_offsets_and_value(*import.get_lookup(), true);
			}

			fmt.get_stream() << ' ';
			fmt.print_field_name("IAT");
			fmt.print_offsets_and_value(import.get_address(), true);

			fmt.get_stream() << " | ";
			std::visit([&fmt] (const auto& info) { dump_import_info(fmt, info); },
				import.get_import_info());

			fmt.print_errors(import);

			fmt.get_stream() << '\n';
		}

		fmt.get_stream() << "\n";
	}
}
catch (const std::system_error& e)
{
	fmt.print_error("Error processing imports:", e);
}
catch (const std::exception& e)
{
	fmt.print_error("Error processing imports:", e);
}

} //namespace

void dump_imports(formatter& fmt, const pe_bliss::image::image& image) try
{
	auto imports = pe_bliss::imports::load(image, {});
	if (!imports)
		return;

	fmt.get_stream() << "===== ";
	fmt.print_structure_name("Import directory");
	fmt.get_stream() << " =====\n\n";
	fmt.print_errors(*imports);
	std::visit([&fmt] (const auto& list) { dump_imports(fmt, list); }, imports->get_list());
}
catch (const std::system_error& e)
{
	fmt.print_error("Error loading imports:", e);
}
catch (const std::exception& e)
{
	fmt.print_error("Error loading imports:", e);
}
