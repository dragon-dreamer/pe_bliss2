#pragma once

class formatter;
namespace pe_bliss { class file_header; }

void dump_file_header(formatter& fmt, const pe_bliss::file_header& header);
