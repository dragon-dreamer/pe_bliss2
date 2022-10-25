#include "section_table_dumper.h"

#include <array>
#include <cstddef>
#include <functional>

#include "formatter.h"

#include "pe_bliss2/section/section_header.h"
#include "pe_bliss2/section/section_table.h"

namespace
{

void dump_section_characteristics(formatter& fmt,
	const pe_bliss::section::section_header& header,
	std::size_t left_padding)
{
	using enum pe_bliss::section::section_header::characteristics::value;
	fmt.print_flags(header.get_characteristics(), left_padding, {
		{ mem_discardable, "MEM_DISCARDABLE" },
		{ mem_not_cached, "MEM_NOT_CACHED" },
		{ mem_not_paged, "MEM_NOT_PAGED" },
		{ mem_shared, "MEM_SHARED" },
		{ mem_execute, "MEM_EXECUTE" },
		{ mem_read, "MEM_READ" },
		{ mem_write, "MEM_WRITE" },
		{ cnt_code, "CNT_CODE" },
		{ cnt_initialized_data, "CNT_INITIALIZED_DATA" },
		{ cnt_uninitialized_data, "CNT_UNINITIALIZED_DATA" }
	});
}

void dump_section_name(formatter& fmt, const pe_bliss::section::section_header& header)
{
	//TODO: can have nullbytes in the middle
	fmt.print_string(std::string(header.get_name()).c_str());
}

void dump_section_header(formatter& fmt, const pe_bliss::section::section_header& header)
{
	fmt.print_structure("Section header", header.get_descriptor(), std::array{
		value_info{"name", true,
			std::bind(dump_section_name, std::ref(fmt), std::cref(header)), false},
		value_info{"virtual_size"},
		value_info{"virtual_address"},
		value_info{"size_of_raw_data"},
		value_info{"pointer_to_raw_data"},
		value_info{"pointer_to_relocations"},
		value_info{"pointer_to_line_numbers"},
		value_info{"number_of_relocations"},
		value_info{"number_of_line_numbers"},
		value_info{"characteristics", true, std::bind(dump_section_characteristics,
			std::ref(fmt), std::cref(header), std::placeholders::_1)}
	});
}

} //namespace

void dump_section_table(formatter& fmt, const pe_bliss::section::section_table& table)
{
	if (table.get_section_headers().empty())
		return;

	fmt.get_stream() << "===== ";
	fmt.print_structure_name("Section table");
	fmt.get_stream() << " =====\n\n";

	for (const auto& header : table.get_section_headers())
		dump_section_header(fmt, header);
}
