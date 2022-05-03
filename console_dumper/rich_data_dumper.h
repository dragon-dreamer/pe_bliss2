#pragma once

class formatter;

namespace pe_bliss
{
class image;
} //namespace pe_bliss

void dump_rich_data(formatter& fmt, const pe_bliss::image& image);
