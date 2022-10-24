#pragma once

#include <cstdint>

namespace pe_bliss::detail::debug
{

struct image_debug_directory
{
	std::uint32_t characteristics;
	std::uint32_t time_date_stamp;
	std::uint16_t major_version;
	std::uint16_t minor_version;
	std::uint32_t type;
	std::uint32_t size_of_data;
	std::uint32_t address_of_raw_data;
	std::uint32_t pointer_to_raw_data;
};

namespace image_debug_type
{
constexpr std::uint32_t unknown = 0u;
//The COFF debug information (line numbers, symbol table, and string table).
//This type of debug information is also pointed to by fields in the file headers. 
constexpr std::uint32_t coff = 1u;
//The Visual C++ debug information. 
constexpr std::uint32_t codeview = 2u;
//The frame pointer omission (FPO) information.
//This information tells the debugger how to interpret nonstandard stack frames,
//which use the EBP register for a purpose other than as a frame pointer.
//If the Type field is set to IMAGE_DEBUG_TYPE_FPO,
//the debug raw data is an array in which each member describes the stack frame of a function.
//Not every function in the image file must have FPO information defined for it,
//even though debug type is FPO.
//Those functions that do not have FPO information are assumed to have normal stack frames.
constexpr std::uint32_t fpo = 3u;
//The location of DBG file.
constexpr std::uint32_t misc = 4u;
//A copy of .pdata section.
constexpr std::uint32_t exception = 5u;
//Reserved.
constexpr std::uint32_t fixup = 6u;
//The mapping from an RVA in image to an RVA in source image.
constexpr std::uint32_t omap_to_src = 7u;
//The mapping from an RVA in source image to an RVA in image.
constexpr std::uint32_t omap_from_src = 8u;
//Reserved for Borland.
constexpr std::uint32_t borland = 9u;
//Reserved.
constexpr std::uint32_t reserved10 = 10u;
//Basic Block Transformation.
constexpr std::uint32_t bbt = reserved10;
//Reserved.
constexpr std::uint32_t clsid = 11u;
constexpr std::uint32_t vc_feature = 12u;
constexpr std::uint32_t pogo = 13u; //aka coffgrp
constexpr std::uint32_t iltcg = 14u; //additionally present and empty when /LTCG:incremental is selected
constexpr std::uint32_t mpx = 15u;
//PE determinism or reproducibility.
//The presence of an entry of type IMAGE_DEBUG_TYPE_REPRO indicates the PE file is built
//in a way to achieve determinism or reproducibility. If the input does not change,
//the output PE file is guaranteed to be bit-for-bit identical no matter when or where the PE is produced.
//Various date/time stamp fields in the PE file are filled with part or all the bits
//from a calculated hash value that uses PE file content as input,
//and therefore no longer represent the actual date and time when
//a PE file or related specific data within the PE is produced.
//The raw data of this debug entry may be empty,
//or may contain a calculated hash value preceded by a four-byte value that represents the hash value length.
constexpr std::uint32_t repro = 16u;
//contains a null-terminated unicode (2-byte) string
//likely related to sample profile guided optimization (/SPGO and /SPD:sample_profile_file options)
constexpr std::uint32_t spgo = 18u;
//Extended DLL characteristics bits.
//If the Type field is set to IMAGE_DEBUG_TYPE_EX_DLLCHARACTERISTICS,
//the debug raw data contains extended DLL characteristics bits,
//in additional to those that could be set in image's optional header.
constexpr std::uint32_t ex_dllcharacteristics = 20u;
} //namespace image_debug_type

namespace image_dllcharacteristics_ex
{
//Reference: https://windows-internals.com/cet-updates-cet-on-xanax/
//Image is CET compatible (shadow stack).
constexpr std::uint32_t cet_compat = 0x01u;
//Blocks binaries that were not compiled with CET support from being loaded into the process.
constexpr std::uint32_t cet_compat_strict_mode = 0x02u;
//When implementing CET support, Microsoft ran into a problem.
//NtSetContextThread is widely used across the system by processes that don't
//necessarily respect the new "rules" of CET, and might use it to set RIP to addresses
//that are not found in the shadow stack. Those processes might also unwind into addresses
//that are not considered valid by CET, and since they were not compiled with proper CET suppor
//they will not have Static nor Dynamic Exception Handler Continuation Targets
//that are recognized by CET. It won't be possible to enable CET across the system
//without breaking all those processes, some of which, like python, are very common.
//So, an option was added to "relax" CetSetContextIpValidation for those cases.
//For any process where "relaxed mode" is enabled,
//setting the context or unwinding into JIT'ed code will always be permitted.
constexpr std::uint32_t cet_set_context_ip_validation_relaxed_mode = 0x04u;
//At first CET was supposed to block all non-approved RIP changes.
//That was too much, so it was toned down to only block most non-approved RIP targets.
//Then MS remembered dynamic memory, and couldn't force dynamic memory to comply with
//CET but insisted that allowing dynamic targets was only supported out of proc,
//so not really a security risk. And now it seems that in proc dynamic APIs are allowed
//by default and processes have to manually opt-out of that by setting this flag.
constexpr std::uint32_t cet_dynamic_apis_allow_in_proc = 0x08u;
constexpr std::uint32_t cet_reserved_1 = 0x10u; //reserved for cet policy *downgrade* only!
constexpr std::uint32_t cet_reserved_2 = 0x20u; //reserved for cet policy *downgrade* only!
} //namespace image_dllcharacteristics_ex

struct image_coff_symbols_header
{
	std::uint32_t number_of_symbols;
	std::uint32_t lva_to_first_symbol;
	std::uint32_t number_of_linenumbers;
	std::uint32_t lva_to_first_linenumber;
	std::uint32_t rva_to_first_byte_of_code;
	std::uint32_t rva_to_last_byte_of_code;
	std::uint32_t rva_to_first_byte_of_data;
	std::uint32_t rva_to_last_byte_of_data;
};

struct coff_symbol
{
	std::uint8_t name[8u];
	//or union with:
	//unsigned long e_zeroes;
	//unsigned long e_offset;

	std::uint32_t value;
	std::int16_t section_number;
	std::uint16_t type;
	std::uint8_t storage_class;
	std::uint8_t aux_symbol_number;
};

constexpr std::int16_t coff_section_number_undef = 0;
constexpr std::int16_t coff_section_number_absolute = -1;
constexpr std::int16_t coff_section_number_debug = -2;

constexpr std::uint8_t coff_type_null = 0;
constexpr std::uint8_t coff_type_void = 1;
constexpr std::uint8_t coff_type_char = 2;
constexpr std::uint8_t coff_type_short = 3;
constexpr std::uint8_t coff_type_int = 4;
constexpr std::uint8_t coff_type_long = 5;
constexpr std::uint8_t coff_type_float = 6;
constexpr std::uint8_t coff_type_double = 7;
constexpr std::uint8_t coff_type_struct = 8;
constexpr std::uint8_t coff_type_union = 9;
constexpr std::uint8_t coff_type_enum = 10;
constexpr std::uint8_t coff_type_member_of_enum = 11;
constexpr std::uint8_t coff_type_uchar = 12;
constexpr std::uint8_t coff_type_ushort = 13;
constexpr std::uint8_t coff_type_uint = 14;
constexpr std::uint8_t coff_type_ulong = 15;
constexpr std::uint8_t coff_type_mask = 0xffu;
constexpr std::uint8_t coff_type_long_double = 16;

constexpr std::uint16_t coff_type_attr_mask = 0x300u;
constexpr std::uint16_t coff_type_no_derived = 0u;
constexpr std::uint16_t coff_type_pointer = 0x100u;
constexpr std::uint16_t coff_type_function_return_value = 0x200u;
constexpr std::uint16_t coff_type_array = 0x300u;

constexpr std::uint8_t coff_storage_class_null = 0u;
constexpr std::uint8_t coff_storage_class_automatic = 1u;
constexpr std::uint8_t coff_storage_class_external = 2u;
constexpr std::uint8_t coff_storage_class_static = 3u;
constexpr std::uint8_t coff_storage_class_register = 4u;
constexpr std::uint8_t coff_storage_class_external_definition = 5u;
constexpr std::uint8_t coff_storage_class_label = 6u;
constexpr std::uint8_t coff_storage_class_undefined_label = 7u;
constexpr std::uint8_t coff_storage_class_member_of_structure = 8u;
constexpr std::uint8_t coff_storage_class_function_argument = 9u;
constexpr std::uint8_t coff_storage_class_structure_tag = 10u;
constexpr std::uint8_t coff_storage_class_member_of_union = 11u;
constexpr std::uint8_t coff_storage_class_union_tag = 12u;
constexpr std::uint8_t coff_storage_class_type_definition = 13u;
constexpr std::uint8_t coff_storage_class_undefined_static = 14u;
constexpr std::uint8_t coff_storage_class_enumeration_tag = 15u;
constexpr std::uint8_t coff_storage_class_member_of_enumeration = 16u;
constexpr std::uint8_t coff_storage_class_register_param = 17u;
constexpr std::uint8_t coff_storage_class_bit_field = 18u;
constexpr std::uint8_t coff_storage_class_automatic_argument = 19u;
constexpr std::uint8_t coff_storage_class_dummy_entry = 20u;
constexpr std::uint8_t coff_storage_class_begin_or_end_of_block = 100u;
constexpr std::uint8_t coff_storage_class_begin_or_end_of_function = 101u;
constexpr std::uint8_t coff_storage_class_end_of_structure = 102u;
constexpr std::uint8_t coff_storage_class_file_name = 103u;
constexpr std::uint8_t coff_storage_class_line_number = 104u;
constexpr std::uint8_t coff_storage_class_duplicate_tag = 105u;
constexpr std::uint8_t coff_storage_class_hidden = 106u;
constexpr std::uint8_t coff_storage_class_physical_end_of_function = 255u;

constexpr std::uint32_t frame_type_fpo = 0u;
constexpr std::uint32_t frame_type_trap = 1u;
constexpr std::uint32_t frame_type_tss = 2u;
constexpr std::uint32_t frame_type_nonfpo = 3u;

struct fpo_data
{
	std::uint32_t ul_off_start; //offset 1st byte of function code
	std::uint32_t cb_proc_size; //# bytes in function
	std::uint32_t cdw_locals; //# bytes in locals/4
	std::uint16_t cdw_params; //# bytes in params/4
	std::uint16_t flags;
	/*
	//flags:
	std::uint16_t cb_prolog : 8; //# bytes in prolog
	std::uint16_t cb_regs : 3; //# regs saved
	std::uint16_t f_has_seh : 1; //TRUE if SEH in func
	std::uint16_t f_use_bp : 1; //TRUE if EBP has been allocated
	std::uint16_t reserved : 1; //reserved for future use
	std::uint16_t cb_frame : 2; //frame type
	*/
};

constexpr std::uint32_t image_debug_misc_exename = 1u;

struct image_debug_misc
{
	std::uint32_t data_type; //type of misc data, see defines
	std::uint32_t length; //total length of record, rounded to four byte multiple.
	std::uint8_t unicode; //TRUE if data is unicode string
	std::uint8_t reserved[3];
	// std::uint8_t data[1]; //actual data
};

struct image_debug_dllcharacteristics_ex
{
	std::uint32_t flags; //image_dllcharacteristics_ex flags
};

constexpr std::uint32_t debug_signature_pdb7 = 0x53445352u; //RSDS
//VC++ 2.0+
constexpr std::uint32_t debug_signature_pdb2 = 0x3031424eu; //NB10
//link 5.20 or earlier linker
constexpr std::uint32_t debug_signature_nb02 = 0x3230424eu;
//link 5.30 and not cvpacked
constexpr std::uint32_t debug_signature_nb05 = 0x3530424eu;
//ilink 1.30 and not cvpacked
constexpr std::uint32_t debug_signature_nb06 = 0x3630424eu;
//cvpacked by QCWIN 1.0
constexpr std::uint32_t debug_signature_nb07 = 0x3730424eu;
//cvpacked for C7.0
constexpr std::uint32_t debug_signature_nb08 = 0x3830424eu;
//cvpacked for C8.0
constexpr std::uint32_t debug_signature_nb09 = 0x3930424eu;
//cvpacked for C11.0
constexpr std::uint32_t debug_signature_nb11 = 0x3131424eu;

struct omf_signature
{
	std::uint8_t signature[4]; //"NBxx"
	std::uint32_t filepos; //offset in file
};

struct cv_info_pdb20
{
	std::uint32_t cv_signature;
	std::uint32_t offset;
	std::uint32_t signature; //unix timestamp
	std::uint32_t age;
	//std::uint8_t pdb_file_name[1];
};

struct cv_info_pdb70
{
	std::uint32_t cv_signature;
	std::uint8_t signature[16]; //GUID
	std::uint32_t age;
	//std::uint8_t pdb_file_name[1];
};

struct image_debug_repro
{
	std::uint32_t length; //hash length
	//std::uint8_t hash[1]; //hash value
};

struct image_debug_vc_feature
{
	std::uint32_t pre_vc_plus_plus_11_count;
	std::uint32_t c_and_c_plus_plus_count;
	std::uint32_t gs_count; // /GS
	std::uint32_t sdl_count; // /sdl
	std::uint32_t guard_n_count;
};

constexpr std::uint32_t mpx_signature = 0x5042524au; //JRBP

struct image_debug_mpx
{
	std::uint32_t signature;
	std::uint32_t unknown1;
	std::uint32_t flags;
	std::uint32_t unknown2;
	std::uint32_t unknown3;
};

namespace pogo_type
{
constexpr std::uint32_t ltcg = 0x4c544347u; //Link-time code generation
constexpr std::uint32_t pgu = 0x50475500u; //Profile-guided optimization - update
constexpr std::uint32_t pgi = 0x50474900u; //Profile-guided optimization - instrument
constexpr std::uint32_t pgo = 0x50474f00u; //Profile-guided optimization - optimize
} //namespace pogo_type

struct pogo_header
{
	std::uint32_t signature; //pogo_type
	//pogo_entry items[1];
};

struct pogo_entry
{
	std::uint32_t start_rva;
	std::uint32_t size;
	//char name[1];
};

struct image_debug_omap
{
	std::uint32_t rva;
	std::uint32_t rva_to;
};

} //namespace pe_bliss::detail::debug
