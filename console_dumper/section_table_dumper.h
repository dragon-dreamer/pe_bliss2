#pragma once

class formatter;
namespace pe_bliss { class section_table; }

void dump_section_table(formatter& fmt, const pe_bliss::section_table& table);
