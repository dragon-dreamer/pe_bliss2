#pragma once

#include <cstdint>

#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/pe_types.h"

namespace pe_bliss::detail::delay_import
{

template<executable_pointer Pointer>
using image_thunk_data = Pointer;

struct image_delayload_descriptor
{
    //std::uint32_t rva_based : 1; //Delay load version 2
    //std::uint32_t reserved_attributes : 31;
    std::uint32_t all_attributes;

    //RVA to the name of the target library (NULL-terminate ASCII string)
    std::uint32_t name;
    //RVA to the HMODULE caching location (PHMODULE)
    std::uint32_t module_handle_rva;
    //RVA to the start of the IAT (image_thunk_data)
    std::uint32_t address_table;
    //RVA to the start of the name table (image_thunk_data::address_of_data)
    std::uint32_t lookup_table;
    //RVA to an optional bound IAT
    std::uint32_t bound_import_address_table_rva;
    //RVA to an optional unload info table
    std::uint32_t unload_information_table_rva;
    //0 if not bound, otherwise, date/time of the target DLL
    std::uint32_t time_date_stamp;
};

} //namespace pe_bliss::detail::delay_import
