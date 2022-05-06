#pragma once

class formatter;
namespace pe_bliss::section { class section_table; }

void dump_section_table(formatter& fmt,
	const pe_bliss::section::section_table& table);
