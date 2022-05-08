#pragma once

class formatter;
namespace pe_bliss::core { class file_header; }

void dump_file_header(formatter& fmt,
	const pe_bliss::core::file_header& header);
