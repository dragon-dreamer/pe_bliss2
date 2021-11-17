#pragma once

class formatter;
namespace pe_bliss { class optional_header; }

void dump_optional_header(formatter& fmt, const pe_bliss::optional_header& header);
