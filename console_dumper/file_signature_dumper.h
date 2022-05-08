#pragma once

class formatter;
namespace pe_bliss::core { class image_signature; }

void dump_file_signature(formatter& fmt,
	const pe_bliss::core::image_signature& signature);
