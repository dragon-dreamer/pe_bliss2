#pragma once

class formatter;
namespace pe_bliss { class image; }

void dump_exports(formatter& fmt, const pe_bliss::image& image);
