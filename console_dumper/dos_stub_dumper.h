#pragma once

class formatter;
namespace pe_bliss { class dos_stub; }

void dump_dos_stub(formatter& fmt, const pe_bliss::dos_stub& stub);
