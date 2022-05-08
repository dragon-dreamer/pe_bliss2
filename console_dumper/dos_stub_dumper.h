#pragma once

class formatter;
namespace pe_bliss::dos { class dos_stub; }

void dump_dos_stub(formatter& fmt, const pe_bliss::dos::dos_stub& stub);
