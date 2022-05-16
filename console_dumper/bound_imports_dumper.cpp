#include "bound_imports_dumper.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include "formatter.h"

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/bound_import/bound_import_directory_loader.h"
#include "pe_bliss2/bound_import/bound_library.h"

namespace
{

template<typename Library>
void dump_descriptor(const char* name, const Library& library, formatter& fmt)
{
	fmt.print_errors(library);
	fmt.print_structure(name, library.get_descriptor(), std::array{
		value_info{"time_date_stamp"},
		value_info{"offset_module_name", true, std::bind(&formatter::print_packed_string, std::ref(fmt),
			std::cref(library.get_library_name()))},
		value_info{"number_of_module_forwarder_refs"}
	});
}

} //namespace

void dump_bound_imports(formatter& fmt, const pe_bliss::image::image& image) try
{
	auto bound_imports = pe_bliss::bound_import::load(image, {});
	if (!bound_imports)
		return;

	fmt.get_stream() << "===== ";
	fmt.print_structure_name("Bound import directory");
	fmt.get_stream() << " =====\n\n";

	for (const auto& library : *bound_imports)
	{
		dump_descriptor("Bound library descriptor", library, fmt);

		std::size_t index = 1;
		for (const auto& ref : library.get_references())
		{
			dump_descriptor(("Bound library forwarder ref #"
				+ std::to_string(index++)).c_str(), ref, fmt);
		}
	}
}
catch (const std::system_error& e)
{
	fmt.print_error("Error loading bound imports:", e);
}
catch (const std::exception& e)
{
	fmt.print_error("Error loading bound imports:", e);
}
