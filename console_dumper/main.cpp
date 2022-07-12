#include <exception>
#include <iostream>

#include "color_provider.h"
#include "bound_imports_dumper.h"
#include "dos_header_dumper.h"
#include "dos_stub_dumper.h"
#include "exceptions_dumper.h"
#include "exports_dumper.h"
#include "file_header_dumper.h"
#include "file_signature_dumper.h"
#include "formatter.h"
#include "image_factory.h"
#include "imports_dumper.h"
#include "load_config_dumper.h"
#include "optional_header_dumper.h"
#include "relocations_dumper.h"
#include "rich_data_dumper.h"
#include "section_table_dumper.h"
#include "tls_dumper.h"

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/image/image.h"

//TODO: configuration flags, output options
void dump_pe(const pe_bliss::image::image& image)
{
	//escape_sequence_color_provider color_provider;
	empty_color_provider color_provider;
	formatter fmt(color_provider, std::cout, std::cerr);
	dump_dos_header(fmt, image.get_dos_header());
	dump_dos_stub(fmt, image.get_dos_stub());
	dump_rich_data(fmt, image);
	dump_file_signature(fmt, image.get_image_signature());
	dump_file_header(fmt, image.get_file_header());
	dump_optional_header(fmt, image.get_optional_header());
	dump_section_table(fmt, image.get_section_table());
	dump_exports(fmt, image);
	dump_imports(fmt, image);
	dump_bound_imports(fmt, image);
	dump_tls(fmt, image);
	dump_relocations(fmt, image);
	dump_load_config(fmt, image);
	dump_exceptions(fmt, image);
}

int main(int argc, char* argv[]) try
{
	if (argc < 2)
	{
		std::cout << "Usage: console_dumper.exe [pe_file]\n";
		return -1;
	}
	const char* filename = argv[1];

	auto result = load_image(filename, {});
	if (result.warnings.has_errors())
	{
		empty_color_provider color_provider;
		formatter fmt(color_provider, std::cout, std::cerr);
		fmt.print_errors(result.warnings);
	}

	if (!result)
	{
		empty_color_provider color_provider;
		formatter fmt(color_provider, std::cout, std::cerr);
		try
		{
			std::rethrow_exception(result.fatal_error);
		}
		catch (const pe_bliss::pe_error& e)
		{
			fmt.print_error("Error loading PE: ", e);
		}
		catch (const std::exception& e)
		{
			fmt.print_error("Error loading PE: ", e);
		}

		return -2;
	}

	dump_pe(result.image);
}
catch (const std::system_error& e)
{
	std::cerr << "Error: " << e.code() << ", " << e.what() << "\n\n";
	return -3;
}
catch (const std::exception& e)
{
	std::cerr << "Error: " << e.what() << '\n';
	return -3;
}
