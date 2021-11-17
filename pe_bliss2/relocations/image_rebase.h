#pragma once

#include <cstdint>

#include "pe_bliss2/relocations/base_relocation.h"

namespace pe_bliss
{
class image;
}

namespace pe_bliss::relocations
{

void rebase(image& instance, const base_relocation_details_list& relocs, std::uint64_t new_base);
void rebase(image& instance, const base_relocation_list& relocs, std::uint64_t new_base);

} //namespace pe_bliss::relocations
