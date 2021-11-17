#pragma once

class formatter;

namespace pe_bliss
{
class dos_stub;
class rich_header;
} //namespace pe_bliss

void dump_rich_data(formatter& fmt, const pe_bliss::dos_stub& stub,
	const pe_bliss::rich_header& header);
