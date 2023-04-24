#pragma once

#include <cstdint>

#include "pe_bliss2/pe_types.h"

namespace pe_bliss::detail::delay_import
{

struct image_delayload_descriptor
{
    //std::uint32_t rva_based : 1; //Delay load version 2
    //std::uint32_t reserved_attributes : 31;
    std::uint32_t all_attributes;

    //RVA to the name of the target library (NULL-terminate ASCII string)
    rva_type name;
    //RVA to the HMODULE caching location (PHMODULE)
    rva_type module_handle_rva;
    //RVA to the start of the IAT (image_thunk_data)
    rva_type address_table;
    //RVA to the start of the name table (image_thunk_data::address_of_data)
    rva_type lookup_table;
    //RVA to an optional bound IAT
    rva_type bound_import_address_table_rva;
    //RVA to an optional unload info table
    rva_type unload_information_table_rva;
    //0 if not bound, otherwise, date/time of the target DLL
    rva_type time_date_stamp;
};

} //namespace pe_bliss::detail::delay_import
