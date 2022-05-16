#pragma once

class formatter;
namespace pe_bliss::image { class image; }

void dump_exports(formatter& fmt, const pe_bliss::image::image& image);
