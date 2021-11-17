#pragma once

class formatter;
namespace pe_bliss { class image_signature; }

void dump_file_signature(formatter& fmt, const pe_bliss::image_signature& signature);
