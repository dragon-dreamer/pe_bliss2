#pragma once

class formatter;
namespace pe_bliss::core { class optional_header; }

void dump_optional_header(formatter& fmt,
	const pe_bliss::core::optional_header& header);
