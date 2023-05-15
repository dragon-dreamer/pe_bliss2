#pragma once

#include <cstdint>

namespace pe_bliss::detail::exceptions
{

struct image_ce_runtime_function_entry
{
    std::uint32_t func_start;
    std::uint32_t metadata;
    /*
    metadata contents:
    std::uint32_t prolog_len : 8;
    std::uint32_t func_len : 22;
    std::uint32_t thirty_two_bit : 1;
    std::uint32_t exception_flag : 1;
    */
};

struct image_arm_runtime_function_entry
{
    std::uint32_t begin_address;
    std::uint32_t unwind_data;
    /*
    union
    {
        std::uint32_t unwind_data;
        struct
        {
            std::uint32_t flag : 2;
            std::uint32_t function_length : 11;
            std::uint32_t ret : 2;
            std::uint32_t h : 1;
            std::uint32_t reg : 3;
            std::uint32_t r : 1;
            std::uint32_t l : 1;
            std::uint32_t c : 1;
            std::uint32_t stack_adjust : 10;
        };
    };
    */
};

enum class arm64_fnpdata_flags
{
    pdata_ref_to_full_xdata = 0,
    pdata_packed_unwind_function = 1,
    pdata_packed_unwind_fragment = 2
};

enum class arm64_fnpdata_cr
{
    pdata_cr_unchained = 0,
    pdata_cr_unchained_saved_lr = 1,
    pdata_cr_chained_with_pac = 2,
    pdata_cr_chained = 3
};

struct image_arm64_runtime_function_entry
{
    std::uint32_t begin_address;
    std::uint32_t unwind_data;
    /*
    union
    {
        std::uint32_t unwind_data;
        struct
        {
            std::uint32_t flag : 2;
            std::uint32_t function_length : 11;
            std::uint32_t reg_f : 3;
            std::uint32_t reg_i : 4;
            std::uint32_t h : 1;
            std::uint32_t cr : 2;
            std::uint32_t frame_size : 9;
        };
    };
    */
};

struct image_arm64_runtime_function_entry_xdata
{
    std::uint32_t header_data;
    /*
    header_data contents:
    struct
    {
        std::uint32_t function_length : 18; // in words (2 bytes)
        std::uint32_t version : 2;
        std::uint32_t exception_data_present : 1;
        std::uint32_t epilog_in_header : 1;
        std::uint32_t epilog_count : 5; // number of epilogs or byte index of the first unwind code for the one only epilog
        std::uint32_t code_words : 5; // number of dwords with unwind codes
    };
    */
};

struct image_alpha64_runtime_function_entry
{
    std::uint64_t begin_address;
    std::uint64_t end_address;
    std::uint64_t exception_handler;
    std::uint64_t handler_data;
    std::uint64_t prolog_end_address;
};

struct image_alpha_runtime_function_entry
{
    std::uint32_t begin_address;
    std::uint32_t end_address;
    std::uint32_t exception_handler;
    std::uint32_t handler_data;
    std::uint32_t prolog_end_address;
};

//The RUNTIME_FUNCTION structure must be DWORD aligned in memory.
struct image_runtime_function_entry
{
    std::uint32_t begin_address;
    std::uint32_t end_address;
    std::uint32_t unwind_info_address;
};

//The UNWIND_INFO structure must be DWORD aligned in memory.
struct unwind_info
{
    std::uint8_t flags_and_version; //3 lower bits version, 5 upper bits flags
    std::uint8_t size_of_prolog;
    std::uint8_t count_of_unwind_codes;
    std::uint8_t frame_register_and_offset; //4 lower bits FR, 4 upper bits FR offset (scaled)
    //std::uint8_t array of unwind codes, aligned to even amount
    //either exception handler or chained unwind info
};

template<std::uint8_t Nodes>
struct unwind_code
{
};

template<>
struct unwind_code<0u>
{
    std::uint8_t offset_in_prolog;
    std::uint8_t unwind_operation_code_and_info; //4 lower bits - code, 4 upper bits - info
};

template<>
struct unwind_code<1u>
{
    std::uint8_t offset_in_prolog;
    std::uint8_t unwind_operation_code_and_info; //4 lower bits - code, 4 upper bits - info
    std::uint16_t node;
};

template<>
struct unwind_code<2u>
{
    std::uint8_t offset_in_prolog;
    std::uint8_t unwind_operation_code_and_info; //4 lower bits - code, 4 upper bits - info
    std::uint32_t node;
};

constexpr std::uint8_t unw_flag_ehandler = 0x1u;
constexpr std::uint8_t unw_flag_uhandler = 0x2u;
constexpr std::uint8_t unw_flag_chaininfo = 0x4u;

constexpr std::uint32_t unwind_chain_limit = 32u;

//struct scope_table
//std::uint32_t count;
//then array of scope_record[count]
struct scope_record
{
    std::uint32_t begin_address;
    std::uint32_t end_address;
    std::uint32_t handler_address;
    std::uint32_t jump_target;
};

} //namespace pe_bliss::detail::exceptions
