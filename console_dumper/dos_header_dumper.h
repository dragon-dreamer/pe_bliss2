#pragma once

class formatter;
namespace pe_bliss::dos { class dos_header; }

void dump_dos_header(formatter& fmt, const pe_bliss::dos::dos_header& header);
