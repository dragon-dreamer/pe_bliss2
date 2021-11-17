#pragma once

class formatter;
namespace pe_bliss { class dos_header; }

void dump_dos_header(formatter& fmt, const pe_bliss::dos_header& header);
