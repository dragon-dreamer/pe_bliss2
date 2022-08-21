#pragma once

#include <cstdint>

#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/pe_types.h"

namespace pe_bliss::detail::load_config
{

struct image_load_config_directory_base32
{
	//The date and time stamp value.
	//The value is represented in the number of seconds elapsed since midnight (00:00:00), January 1, 1970, Universal Coordinated Time,
	//according to the system clock.
	//The time stamp can be printed using the C run-time (CRT) function ctime.
	std::uint32_t time_date_stamp;
	std::uint16_t major_version;
	std::uint16_t minor_version;
	//The global flags that control system behavior. For more information, see Gflags.exe.
	std::uint32_t global_flags_clear;
	std::uint32_t global_flags_set;
	std::uint32_t critical_section_default_timeout;
	//The size of the minimum block that must be freed before it is freed (de-committed), in bytes.
	//This value is advisory.
	std::uint32_t de_commit_free_block_threshold;
	//The size of the minimum total memory that must be freed in the process heap before it is
	//freed (de-committed), in bytes. This value is advisory.
	std::uint32_t de_commit_total_free_threshold;
	//The VA of a list of addresses where the LOCK prefix is used.
	//These will be replaced by NOP on single-processor systems. This member is available only for x86.
	std::uint32_t lock_prefix_table;
	//The maximum allocation size, in bytes. This member is obsolete and is used only for debugging purposes.
	std::uint32_t maximum_allocation_size;
	//The maximum block size that can be allocated from heap segments, in bytes.
	std::uint32_t virtual_memory_threshold;
	//The process heap flags. For more information, see HeapCreate.
	std::uint32_t process_heap_flags;
	//The process affinity mask. For more information, see GetProcessAffinityMask.
	//This member is available only for .exe files.
	std::uint32_t process_affinity_mask;
	//The service pack version.
	std::uint16_t csd_version;
	//An optional integer value that specifies the load flags to apply when
	//resolving statically linked import dependencies of the module.
	//For a list of supported flag values, see the LOAD_LIBRARY_SEARCH_* entries in LoadLibraryEx.
	std::uint16_t dependent_load_flags;
	//Reserved for use by the system.
	std::uint32_t edit_list;
	//A pointer to a cookie that is used by Visual C++ or GS implementation.
	std::uint32_t security_cookie;
};

struct structured_exceptions32
{
	std::uint32_t se_handler_table;
	std::uint32_t se_handler_count;
};

//Threshold 1 (1507)
struct cf_guard32
{
	std::uint32_t guard_cf_check_function_pointer;
	std::uint32_t guard_cf_dispatch_function_pointer;
	//This is a sorted list of relative virtual addresses (RVA) that contain information about valid CFG call targets.
	//The RVA list in the GFIDS table must be sorted properly or the image will not be loaded.
	//The GFIDS table is an array of 4 + n bytes, where n is given by
	//((GuardFlags & IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_MASK) >> IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_SHIFT).
	//"GuardFlags" is the GuardFlags field of the load configuration directory.
	//This allows for extra metadata to be attached to CFG call targets in the future.
	//The only currently defined metadata is an optional 1-byte extra flags field ("GFIDS flags")
	//that is attached to each GFIDS entry if any call targets have metadata.
	std::uint32_t guard_cf_function_table;
	std::uint32_t guard_cf_function_count;
	std::uint32_t guard_flags;
};

//Threshold 2 (1511) (preview 9879)
struct image_load_config_code_integrity
{
	//Flags to indicate if CI information is available, etc.
	std::uint16_t flags;
	//0xFFFF means not available
	std::uint16_t catalog;
	std::uint32_t catalog_offset;
	std::uint32_t reserved;
};

constexpr std::uint16_t code_integrity_catalog_not_available = 0xffffu;

//Redstone 1 (1607) (build 14286)
//https://docs.microsoft.com/en-us/windows/win32/secbp/pe-metadata#export-suppression
struct cf_guard_ex32
{
	//This table is structurally formatted the same as the GFIDS table. It uses the same GuardFlags
	//IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_MASK mechanism to encode extra optional metadata bytes in the address taken IAT table,
	//though all metadata bytes must be zero for the address taken IAT table and are reserved.
	//The address taken IAT table indicates a sorted array of RVAs of import thunks
	//which have the imported as a symbol address taken call target.
	std::uint32_t guard_address_taken_iat_entry_table;
	std::uint32_t guard_address_taken_iat_entry_count;
	//This table is structurally formatted the same as the GFIDS table and uses the same GuardFlags
	//IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_MASK mechanism to encode optional extra metadata bytes in the long jump table.
	//All metadata bytes must be zero for the long jump table and are reserved.
	//The long jump table represents a sorted array of RVAs that are valid long jump targets.
	std::uint32_t guard_long_jump_target_table;
	std::uint32_t guard_long_jump_target_count;
};

//Redstone 1 (1607) (build 14383)
struct hybrid_pe32
{
	std::uint32_t dynamic_value_reloc_table;
	std::uint32_t chpe_metadata_pointer;
};

//Redstone 2 (1703) (build 14901)
struct rf_guard32
{
	std::uint32_t guard_rf_failure_routine;
	std::uint32_t guard_rf_failure_routine_function_pointer;
	std::uint32_t dynamic_value_reloc_table_offset;
	std::uint16_t dynamic_value_reloc_table_section;
	std::uint16_t reserved2;
};

//Redstone 2 (1703) (build 15002)
struct rf_guard_ex32
{
	std::uint32_t guard_rf_verify_stack_pointer_function_pointer;
	std::uint32_t hot_patch_table_offset;
};

//Redstone 3 (1709) (build 16237)
struct enclave_config32
{
	std::uint32_t reserved3;
	std::uint32_t enclave_configuration_pointer;
};

//Redstone 4 (1803)
struct volatile_metadata32
{
	std::uint32_t volatile_metadata_pointer;
};

//Redstone 5 (1809)
//The /guard:ehcont compiler switch generates an "EH Continuation Table".
//It contains a sorted list of the RVAs of all valid exception handling continuation targets
//in the binary. NtContinue first checks the shadow stack for the user - supplied instruction pointer,
//and if the instruction pointer isn't found there,
//it proceeds to check the EH Continuation Table from the binary that contains the instruction pointer.
struct guard_exception_handling32
{
	std::uint32_t guard_eh_continuation_table;
	std::uint32_t guard_eh_continuation_count;
};

//Vibranium 3 (21H1)
struct extended_flow_guard32
{
	std::uint32_t guard_xfg_check_function_pointer;
	std::uint32_t guard_xfg_dispatch_function_pointer;
	std::uint32_t guard_xfg_table_dispatch_function_pointer;
};

//Vibranium 4 (21H2)
//CastGuard OS determined failure mode
//CastGuard: Checks for static casts of objects to prevent illegal downcasts.
//Causes illegal casts to fast fail.
//Mitigates ~1/3 of reported type confusion cases.
struct cast_guard_os_determined_failure_mode32
{
	std::uint32_t cast_guard_os_determined_failure_mode; // VA
};

//Windows 10 version 22H2 (?)
struct guard_memcpy_function_pointer32
{
	std::uint32_t guard_memcpy_function_pointer; // VA
};

struct image_load_config_directory32
{
	using pointer_type = std::uint32_t;

	/* std::uint32_t size; */
	image_load_config_directory_base32 base;
	structured_exceptions32 structured_exceptions;
	cf_guard32 cf_guard;
	image_load_config_code_integrity code_integrity;
	cf_guard_ex32 cf_guard_ex;
	hybrid_pe32 hybrid_pe;
	rf_guard32 rf_guard;
	rf_guard_ex32 rf_guard_ex;
	enclave_config32 enclave;
	volatile_metadata32 volatile_metadata;
	guard_exception_handling32 guard_exception_handling;
	extended_flow_guard32 extended_flow_guard;
	cast_guard_os_determined_failure_mode32 mode;
	guard_memcpy_function_pointer32 memcpy_function_pointer;
};

struct image_load_config_directory_base64
{
	std::uint32_t time_date_stamp;
	std::uint16_t major_version;
	std::uint16_t minor_version;
	std::uint32_t global_flags_clear;
	std::uint32_t global_flags_set;
	std::uint32_t critical_section_default_timeout;
	std::uint64_t de_commit_free_block_threshold;
	std::uint64_t de_commit_total_free_threshold;
	std::uint64_t lock_prefix_table;
	std::uint64_t maximum_allocation_size;
	std::uint64_t virtual_memory_threshold;
	std::uint64_t process_affinity_mask;
	std::uint32_t process_heap_flags;
	std::uint16_t csd_version;
	std::uint16_t dependent_load_flags;
	std::uint64_t edit_list;
	std::uint64_t security_cookie;
};

struct structured_exceptions64
{
	std::uint64_t se_handler_table;
	std::uint64_t se_handler_count;
};

//Threshold 1 (1507)
struct cf_guard64
{
	std::uint64_t guard_cf_check_function_pointer;
	std::uint64_t guard_cf_dispatch_function_pointer;
	std::uint64_t guard_cf_function_table;
	std::uint64_t guard_cf_function_count;
	std::uint32_t guard_flags;
};

//Redstone 1 (1607) (build 14286)
struct cf_guard_ex64
{
	std::uint64_t guard_address_taken_iat_entry_table;
	std::uint64_t guard_address_taken_iat_entry_count;
	std::uint64_t guard_long_jump_target_table;
	std::uint64_t guard_long_jump_target_count;
};

//Redstone 1 (1607) (build 14383)
struct hybrid_pe64
{
	std::uint64_t dynamic_value_reloc_table;
	std::uint64_t chpe_metadata_pointer;
};

//Redstone 2 (1703) (build 14901)
struct rf_guard64
{
	std::uint64_t guard_rf_failure_routine;
	std::uint64_t guard_rf_failure_routine_function_pointer;
	std::uint32_t dynamic_value_reloc_table_offset;
	std::uint16_t dynamic_value_reloc_table_section;
	std::uint16_t reserved2;
};

//Redstone 2 (1703) (build 15002)
struct rf_guard_ex64
{
	std::uint64_t guard_rf_verify_stack_pointer_function_pointer;
	std::uint32_t hot_patch_table_offset;
};

//Redstone 3 (1709) (build 16237)
struct enclave_config64
{
	std::uint32_t reserved3;
	std::uint64_t enclave_configuration_pointer;
};

//Redstone 4 (1803)
struct volatile_metadata64
{
	std::uint64_t volatile_metadata_pointer;
};

//Redstone 5 (1809)
struct guard_exception_handling64
{
	std::uint64_t guard_eh_continuation_table;
	std::uint64_t guard_eh_continuation_count;
};

//Vibranium 3 (21H1) 
struct extended_flow_guard64
{
	std::uint64_t guard_xfg_check_function_pointer;
	std::uint64_t guard_xfg_dispatch_function_pointer;
	std::uint64_t guard_xfg_table_dispatch_function_pointer;
};

//Vibranium 4 (21H2)
struct cast_guard_os_determined_failure_mode64
{
	std::uint64_t cast_guard_os_determined_failure_mode; // VA
};

//Windows 10 version 22H2 (?)
struct guard_memcpy_function_pointer64
{
	std::uint64_t guard_memcpy_function_pointer; // VA
};

struct image_load_config_directory64
{
	using pointer_type = std::uint64_t;

	/* std::uint32_t size; */
	image_load_config_directory_base64 base;
	structured_exceptions64 structured_exceptions;
	cf_guard64 cf_guard;
	image_load_config_code_integrity code_integrity;
	cf_guard_ex64 cf_guard_ex;
	hybrid_pe64 hybrid_pe;
	rf_guard64 rf_guard;
	rf_guard_ex64 rf_guard_ex;
	enclave_config64 enclave;
	volatile_metadata64 volatile_metadata;
	guard_exception_handling64 guard_exception_handling;
	extended_flow_guard64 extended_flow_guard;
	cast_guard_os_determined_failure_mode64 mode;
	guard_memcpy_function_pointer64 memcpy_function_pointer;
};

struct image_dynamic_relocation_table
{
	std::uint32_t version; //1 (See image_dynamic_relocation) or 2 (see image_dynamic_relocation_v2)
	std::uint32_t size; //Size of the following structures not including this image_dynamic_relocation_table
	//image_dynamic_relocation/image_dynamic_relocation_v2 dynamic_relocations[0];
};

template<executable_pointer Pointer>
struct image_dynamic_relocation
{
	Pointer symbol;
	std::uint32_t base_reloc_size;
	//relocations::image_base_relocation base_relocations[0];
};

using image_dynamic_relocation32 = image_dynamic_relocation<std::uint32_t>;
using image_dynamic_relocation64 = image_dynamic_relocation<std::uint64_t>;

template<executable_pointer Pointer>
struct image_dynamic_relocation_v2
{
	std::uint32_t header_size;
	std::uint32_t fixup_info_size;
	Pointer symbol;
	std::uint32_t symbol_group;
	std::uint32_t flags;
	//... variable length header fields
	//std::uint8_t fixup_info[fixup_info_size]
};

using image_dynamic_relocation32_v2 = image_dynamic_relocation_v2<std::uint32_t>;
using image_dynamic_relocation64_v2 = image_dynamic_relocation_v2<std::uint64_t>;

//Defined symbolic dynamic relocation entries
namespace dynamic_relocation_symbol
{
//Used at runtime with some options. For example, the OS may use these relocations to
//override calls when Retpoline is enabled.
constexpr std::uint32_t guard_rf_prologue = 1u;
constexpr std::uint32_t guard_rf_epilogue = 2u;
constexpr std::uint32_t guard_import_control_transfer = 3u;
constexpr std::uint32_t guard_indir_control_transfer = 4u;
constexpr std::uint32_t guard_switchtable_branch = 5u;
constexpr std::uint32_t guard_arm64x = 6u;
constexpr std::uint32_t function_override = 7u;
} //namespace dynamic_relocation_symbol

//Symbol 1
struct image_prologue_dynamic_relocation_header //Unaligned
{
	std::uint8_t prologue_byte_count;
	//std::uint8_t prologue_bytes[prologue_byte_count];
};

//Symbol 2
//XXX: branch_descriptors, branch_descriptor_bit_map are not documented
struct image_epilogue_dynamic_relocation_header //Unaligned
{
	std::uint32_t epilogue_count;
	std::uint8_t epilogue_byte_count;
	std::uint8_t branch_descriptor_element_size;
	std::uint16_t branch_descriptor_count;
	//std::uint8_t branch_descriptors[...];
	//std::uint8_t branch_descriptor_bit_map[...];
};

//Symbol 3
/*
 IAT function calling patch:
	 call    cs:__imp_PshedFreeMemory
	 nop     dword ptr [rax+rax+00h]
*/
struct image_import_control_transfer_dynamic_relocation //Unaligned
{
	/*
	std::uint32_t page_relative_offset : 12;
	std::uint32_t indirect_call : 1;
	std::uint32_t iat_index : 19;
	*/
	std::uint32_t metadata;
};

//Symbol 4
/*
call reg patch:
	call    rax
	nop     dword ptr [rax]
*/
struct image_indir_control_transfer_dynamic_relocation //Unaligned
{
	/*
	std::uint16_t page_relative_offset : 12;
	std::uint16_t indirect_call : 1;
	std::uint16_t rex_w_prefix : 1;
	std::uint16_t cfg_check : 1;
	std::uint16_t reserved : 1;
	*/
	std::uint16_t metadata;
};

//Symbol 5
/*
call reg in switch patch:
   mov     ecx, ds:rva off_14000DEBC[rdx+rdi*4]
   add     rcx, rdx
   jmp     rcx             ; switch jump
   db 4 dup(0CCh)
*/
struct image_switchtable_branch_dynamic_relocation //Unaligned
{
	/*
	std::uint16_t page_relative_offset : 12;
	std::uint16_t register_number : 4;
	*/
	std::uint16_t metadata;
};

//Symbol 6
//See https://ffri.github.io/ProjectChameleon/new_reloc_chpev2/ and https://github.com/FFRI/ProjectChameleon/
struct image_arm64x_dynamic_relocation
{
	//element size of the array is at least 2 bytes
	//The first two bytes of data of each component is represented by the metadata_and_offset structure,
	//where the offset is an offset from image_dynamic_relocation_arm64x_block::virtual_address for the block,
	//and meta contains the relocation type and other metadata (sign or scale index).
	//The lower two bits of meta specify the relocation type.
	/*
	std::uint16_t page_relative_offset : 12;
	std::uint16_t meta : 4;
	*/
	std::uint16_t metadata;

	//Relocation type 1: zero fill (meta & 0b11 == 0b00)
	//This entry is used to clear the data at the target address to zero.
	//The size is 2^x bytes, where x is the upper two bits of meta

	//Relocation type 2: assign value (meta & 0b11 == 0b01)
	//This entry is used to overwrite the data in the target address with a specified value.
	//The size to be written is encoded in meta in the same manner as for "zero fill".
	//In this relocation entry, metadata_and_offset is followed by data whose size is 2^x bytes,
	//and this data is overwritten to the target address.

	//Relocation type 3: add (or sub) delta (meta & 0b11 == 0b10)
	//This is an entry to add (or subtract) an offset in multiples of four (or eight) to the data in the target address.
	//The scale factor and offset sign are encoded in the upper two bits of the meta, as shown below.
	//meta[3] - scale: 8 for 1, 4 for 0.
	//meta[2] - sign: minus for 1, plus for 0.
	//In this relocation entry, metadata_and_offset is followed by two bytes of data.
	//This value multiplied by the scale factor is added (or subtracted) to the data in the target address.
	
	//std::uint8_t relocation_data[0];
};

//Symbol 7 - undocumented, based on dumpbin and winnt.h
struct image_function_override_header
{
	std::uint32_t func_override_size;
	//image_function_override_dynamic_relocation func_override_info[]; //func_override_size bytes in size
	//image_bdd_info bdd_info; //BDD region, size in bytes:
	//    DVRTEntrySize - sizeof(image_function_override_header) - func_override_size
};

struct image_function_override_dynamic_relocation
{
	std::uint32_t original_rva; //RVA of original function
	std::uint32_t bdd_offset; //Offset into the BDD region
	std::uint32_t rva_size; //Size in bytes taken by RVAs. Must be multiple of sizeof(DWORD)
	std::uint32_t base_reloc_size; //Size in bytes taken by BaseRelocs

	//std::uint32_t RVAs[rva_size / sizeof(DWORD)]; //Array containing overriding func RVAs

	//image_base_relocation base_relocs[]; //Base relocations (RVA + Size + TO)
	                                       //Padded with extra TOs for 4B alignment
										   //base_reloc_size size in bytes
};

struct image_bdd_info
{
	std::uint32_t version; //decides the semantics of serialized BDD
	std::uint32_t bdd_size;
	//image_bdd_dynamic_relocation bdd_nodes[]; //bdd_size size in bytes
};

struct image_bdd_dynamic_relocation
{
	std::uint16_t left; //Index of FALSE edge in BDD array
	std::uint16_t right; //Index of TRUE edge in BDD array
	std::uint32_t value; //Either FeatureNumber or Index into RVAs array
};

//Function override relocation types in DVRT records
namespace image_function_override_type
{
constexpr std::uint32_t invalid = 0u;
constexpr std::uint32_t x64_rel32 = 1u; //32-bit relative address from byte following reloc
constexpr std::uint32_t arm64_branch26 = 2u; //26 bit offset << 2 & sign ext. for B & BL
constexpr std::uint32_t arm64_thunk = 3u;
} //namespace image_function_override_type

struct image_chpe_metadata_x86
{
	/* std::uint32_t version; */
	std::uint32_t cphe_code_address_range_offset;
	std::uint32_t cphe_code_address_range_count;
	std::uint32_t wow_a64_exception_handler_function_pointer;
	std::uint32_t wow_a64_dispatch_call_function_pointer;
	std::uint32_t wow_a64_dispatch_indirect_call_function_pointer;
	std::uint32_t wow_a64_dispatch_indirect_call_cfg_function_pointer;
	std::uint32_t wow_a64_dispatch_ret_function_pointer;
	std::uint32_t wow_a64_dispatch_ret_leaf_function_pointer;
	std::uint32_t wow_a64_dispatch_jump_function_pointer;
	std::uint32_t compiler_iat_pointer; //Present if version >= 2
	std::uint32_t wow_a64_rdtsc_function_pointer; //Present if version >= 3
};

struct image_chpe_x86_range_entry
{
	/*
	union {
		std::uint32_t start_offset;
		struct {
			std::uint32_t native_code : 1;
			std::uint32_t address_bits : 31;
		};
	};
	*/

	//RVA with lower bit indicating code type: 1 - arm64, 0 - x86
	std::uint32_t start_offset;
	std::uint32_t length;
};

constexpr std::uint8_t chpe_x86_range_code_type_arm64 = 0b0u;
constexpr std::uint8_t chpe_x86_range_code_type_x86 = 0b1u;
constexpr std::uint32_t chpe_x86_range_code_type_mask = 0b1u;

//XXX: mostly undocumented
struct image_chpe_metadata_arm64x
{
	/* std::uint32_t version; */
	std::uint32_t cphe_code_address_range_offset;
	std::uint32_t cphe_code_address_range_count;
	std::uint32_t x64_code_ranges_to_entry_points_table;
	std::uint32_t arm64x_redirection_metadata_table;
	std::uint32_t dispatch_call_function_pointer_no_redirection;
	std::uint32_t dispatch_return_function_pointer;
	std::uint32_t unknown_rva1;
	std::uint32_t dispatch_indirect_call_function_pointer;
	std::uint32_t dispatch_indirect_call_function_pointer_with_cfg_check;
	std::uint32_t alternative_entry_point;
	std::uint32_t auxiliary_import_address_table;
	std::uint32_t x64_code_ranges_to_entry_points_table_entry_count;
	std::uint32_t arm64x_redirection_metadata_table_entry_count;
	std::uint32_t unknown_rva2;
	std::uint32_t unknown_rva3;
	//extra_rfe_table points to ARM64 exception unwind information
	std::uint32_t extra_rfe_table;
	std::uint32_t extra_rfe_table_size;
	std::uint32_t dispatch_function_pointer;
	std::uint32_t copy_of_auxiliary_import_address_table;
};

struct image_chpe_arm64x_range_entry
{
	/*
	union {
		std::uint32_t start_offset;
		struct {
			std::uint32_t code_type : 2;
			std::uint32_t address_bits : 30;
		};
	};
	*/

	//RVA with two lower bits indicating code type
	std::uint32_t start_offset;
	std::uint32_t length;
};

constexpr std::uint8_t chpe_arm64x_range_code_type_arm64 = 0b00u;
constexpr std::uint8_t chpe_arm64x_range_code_type_arm64ec = 0b01u;
constexpr std::uint8_t chpe_arm64x_range_code_type_x64 = 0b10u;
constexpr std::uint32_t chpe_arm64x_range_code_type_mask = 0b11u;

//TODO: this is not documented and not researched
struct image_hot_patch_info
{
	std::uint32_t version;
	std::uint32_t size;
	std::uint32_t sequence_number;
	std::uint32_t base_image_list;
	std::uint32_t base_image_count;
	std::uint32_t buffer_offset; //Version 2 and later
	std::uint32_t extra_patch_size; //Version 3 and later
};

struct image_hot_patch_base
{
	std::uint32_t sequence_number;
	std::uint32_t flags;
	std::uint32_t original_time_date_stamp;
	std::uint32_t original_check_sum;
	std::uint32_t code_integrity_info;
	std::uint32_t code_integrity_size;
	std::uint32_t patch_table;
	std::uint32_t buffer_offset; //Version 2 and later
};

struct image_hot_patch_hashes
{
	std::uint8_t sha256[32];
	std::uint8_t sha1[20];
};

constexpr std::uint32_t image_hot_patch_base_obligatory = 0x00000001;
constexpr std::uint32_t image_hot_patch_base_can_roll_back = 0x00000002;

constexpr std::uint32_t image_hot_patch_chunk_inverse = 0x80000000;
constexpr std::uint32_t image_hot_patch_chunk_obligatory = 0x40000000;
constexpr std::uint32_t image_hot_patch_chunk_reserved = 0x3ff03000;
constexpr std::uint32_t image_hot_patch_chunk_type = 0x000fc000;
constexpr std::uint32_t image_hot_patch_chunk_source_rva = 0x00008000;
constexpr std::uint32_t image_hot_patch_chunk_target_rva = 0x00004000;
constexpr std::uint32_t image_hot_patch_chunk_size = 0x00000fff;

constexpr std::uint32_t image_hot_patch_none = 0x00000000;
constexpr std::uint32_t image_hot_patch_function = 0x0001c000;
constexpr std::uint32_t image_hot_patch_absolute = 0x0002c000;
constexpr std::uint32_t image_hot_patch_rel32 = 0x0003c000;
constexpr std::uint32_t image_hot_patch_call_target = 0x00044000;
constexpr std::uint32_t image_hot_patch_indirect = 0x0005c000;
constexpr std::uint32_t image_hot_patch_no_call_target = 0x00064000;
constexpr std::uint32_t image_hot_patch_dynamic_value = 0x00078000;

namespace guard_flags
{
//Module performs control flow integrity checks using system-supplied support
constexpr std::uint32_t cf_instrumented = 0x00000100;
//Module performs control flow and write integrity checks
constexpr std::uint32_t cfw_instrumented = 0x00000200;
//Module contains valid control flow target metadata
constexpr std::uint32_t cf_function_table_present = 0x00000400;
//Module does not make use of the /GS security cookie
constexpr std::uint32_t security_cookie_unused = 0x00000800;
//Module supports read only delay load IAT
//This indicates that the operating system's DLL loader should change protections for the delay load IAT
//during export resolution if using the operating system's delay load support native to Windows 8
//and later operating systems. The synchronization of this step is managed by the operating
//system DLL loader if native operating system delay load support is in use (e.g.ResolveDelayLoadedAPI)
//so no other component should reprotect the pages spanning the declared delay load IAT.
constexpr std::uint32_t protect_delayload_iat = 0x00001000;
//Delayload import table in its own .didat section (with nothing else in it) that can be freely reprotected.
//For backwards compatibility with older pre-CFG operating systems,
//tools may enable the option to move the delay load IAT into its own section (canonically ".didat"),
//protected as read/write in the image headers, and additionally set the
//IMAGE_GUARD_CF_DELAYLOAD_IAT_IN_ITS_OWN_SECTION flag.
//This setting will cause CFG-aware operating system DLL loaders to reprotect the entire section containing
//the IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT table to read only memory during image loading. 
constexpr std::uint32_t delayload_iat_in_its_own_section = 0x00002000;
//Module contains suppressed export information. This also infers that the address taken
//IAT table is also present in the load config.
constexpr std::uint32_t cf_export_suppression_info_present = 0x00004000;
//Module enables suppression of exports
constexpr std::uint32_t cf_enable_export_suppression = 0x00008000;
//Module contains longjmp target information
constexpr std::uint32_t cf_longjump_table_present = 0x00010000;
//Module contains return flow instrumentation and metadata
constexpr std::uint32_t rf_instrumented = 0x00020000;
//Module requests that the OS enable return flow protection
constexpr std::uint32_t rf_enable = 0x00040000;
//Module requests that the OS enable return flow protection in strict mode
constexpr std::uint32_t rf_strict = 0x00080000;
//Module was built with retpoline support
constexpr std::uint32_t retpoline_present = 0x00100000;
//DO_NOT_USE - Was EHCont flag on VB (20H1)
constexpr std::uint32_t eh_continuation_table_present_20h1 = 0x00200000;
//Module contains EH continuation target information
constexpr std::uint32_t eh_continuation_table_present = 0x00400000;
//Module was built with xfg
constexpr std::uint32_t xfg_enabled = 0x00800000;
//Module has CastGuard instrumentation present
constexpr std::uint32_t castguard_present = 0x01000000;
//Module has Guarded Memcpy instrumentation present
constexpr std::uint32_t memcpy_present = 0x02000000;

//Stride of Guard CF function table encoded in these bits (additional count of bytes per element)
constexpr std::uint32_t cf_function_table_size_mask = 0xf0000000;
//Shift to right-justify Guard CF function table stride
constexpr std::uint32_t cf_function_table_size_shift = 28;
} //namespace guard_flags

namespace gfids_flags
{
//Call target is explicitly suppressed (do not treat it as valid for purposes of CFG)
constexpr std::uint8_t fid_suppressed = 1u;
//Call target is export suppressed. See Export suppression for more details.
constexpr std::uint8_t export_suppressed = 2u;
constexpr std::uint8_t fid_langexcpthandler = 4u;
//XF Guard is enabled for this address.
constexpr std::uint8_t fid_xfg = 8u;
} //namespace gfids_flags

constexpr std::uint32_t image_enclave_long_id_length = 32;
constexpr std::uint32_t image_enclave_short_id_length = 16;

template<executable_pointer Pointer>
struct image_enclave_config
{
	//The size of the IMAGE_ENCLAVE_CONFIG structure, in bytes.
	std::uint32_t size;
	//The minimum size of the IMAGE_ENCLAVE_CONFIG structure that the image loader must be able to process
	//in order for the enclave to be usable. This member allows an enclave to inform an earlier version
	//of the image loader that the image loader can safely load the enclave and ignore optional members
	//added to IMAGE_ENCLAVE_CONFIG for later versions of the enclave.
	//If the size of IMAGE_ENCLAVE_CONFIG that the image loader can process is less than MinimumRequiredConfigSize,
	//the enclave cannot be run securely.
	//If MinimumRequiredConfigSize is zero, the minimum size of the IMAGE_ENCLAVE_CONFIG structure
	//that the image loader must be able to process in order for the enclave to be usable is assumed
	//to be the size of the structure through and including the MinimumRequiredConfigSize member.
	std::uint32_t minimum_required_config_size;
	//A flag that indicates whether the enclave permits debugging.
	std::uint32_t policy_flags;
	//The number of images in the array of images that the ImportList member points to.
	std::uint32_t number_of_imports;
	//The relative virtual address of the array of images that the enclave image may import,
	//with identity information for each image.
	std::uint32_t import_list;
	//The size of each image in the array of images that the ImportList member points to.
	std::uint32_t import_entry_size;
	//The family identifier that the author of the enclave assigned to the enclave.
	std::uint8_t family_id[image_enclave_short_id_length];
	//The image identifier that the author of the enclave assigned to the enclave.
	std::uint8_t image_id[image_enclave_short_id_length];
	//The version number that the author of the enclave assigned to the enclave.
	std::uint32_t image_version;
	//The security version number that the author of the enclave assigned to the enclave.
	std::uint32_t security_version;
	//The expected virtual size of the private address range for the enclave, in bytes.
	Pointer enclave_size;
	//The maximum number of threads that can be created within the enclave.
	std::uint32_t number_of_threads;
	//A flag that indicates whether the image is suitable for use as the primary image in the enclave.
	std::uint32_t enclave_flags;
};

using image_enclave_config32 = image_enclave_config<std::uint32_t>;
using image_enclave_config64 = image_enclave_config<std::uint64_t>;

//The enclave permits debugging
constexpr std::uint32_t image_enclave_policy_debuggable = 0x00000001;

//The image is suitable for use as the primary image in the enclave
constexpr std::uint32_t image_enclave_flag_primary_image = 0x00000001;

struct image_enclave_import
{
	//The type of identifier of the image that must match the value in the import record.
	std::uint32_t match_type;
	//The minimum enclave security version that each image must have for the image to be imported successfully.
	//The image is rejected unless its enclave security version is equal to or greater than the minimum
	//value in the import record. Set the value in the import record to zero to turn off the security version check.
	std::uint32_t minimum_security_version;
	//The unique identifier of the primary module for the enclave, if the MatchType member is
	//IMAGE_ENCLAVE_IMPORT_MATCH_UNIQUE_ID. Otherwise, the author identifier of the primary module for the enclave.
	std::uint8_t unique_or_author_id[image_enclave_long_id_length];
	//The family identifier of the primary module for the enclave.
	std::uint8_t family_id[image_enclave_short_id_length];
	//The image identifier of the primary module for the enclave.
	std::uint8_t image_id[image_enclave_short_id_length];
	//The relative virtual address of a NULL-terminated string that contains the same value found in the import directory for the image.
	std::uint32_t import_name;
	//Reserved.
	std::uint32_t reserved;
};

namespace enclave_import_match
{
//None of the identifiers of the image need to match the value in the import record.
constexpr std::uint32_t none = 0x00000000;
//The value of the enclave unique identifier of the image must match the value in the import record.
//Otherwise, loading of the image fails.
constexpr std::uint32_t unique_id = 0x00000001;
//The value of the enclave author identifier of the image must match the value in the import record.
//Otherwise, loading of the image fails.
//If this flag is set and the import record indicates an author identifier of all zeros,
//the imported image must be part of the Windows installation. 
constexpr std::uint32_t author_id = 0x00000002;
//The value of the enclave family identifier of the image must match the value in the import record.
//Otherwise, loading of the image fails.
constexpr std::uint32_t family_id = 0x00000003;
//The value of the enclave image identifier must match the value in the import record.
//Otherwise, loading of the image fails.
constexpr std::uint32_t image_id = 0x00000004;
} //namespace enclave_import_match

namespace gflags
{
constexpr std::uint32_t flg_disable_dbgprint = 0x08000000;
constexpr std::uint32_t flg_kernel_stack_trace_db = 0x2000;
constexpr std::uint32_t flg_user_stack_trace_db = 0x1000;
constexpr std::uint32_t flg_debug_initial_command = 0x04;
constexpr std::uint32_t flg_debug_initial_command_ex = 0x04000000;
constexpr std::uint32_t flg_heap_disable_coalescing = 0x00200000;
constexpr std::uint32_t flg_disable_page_kernel_stacks = 0x080000;
constexpr std::uint32_t flg_disable_protdlls = 0x80000000;
constexpr std::uint32_t flg_disable_stack_extension = 0x010000;
constexpr std::uint32_t flg_critsec_event_creation = 0x10000000;
constexpr std::uint32_t flg_application_verifier = 0x0100;
constexpr std::uint32_t flg_enable_handle_exceptions = 0x40000000;
constexpr std::uint32_t flg_enable_close_exceptions = 0x400000;
constexpr std::uint32_t flg_enable_csrdebug = 0x020000;
constexpr std::uint32_t flg_enable_exception_logging = 0x800000;
constexpr std::uint32_t flg_heap_enable_free_check = 0x20;
constexpr std::uint32_t flg_heap_validate_parameters = 0x40;
constexpr std::uint32_t flg_heap_enable_tagging = 0x0800;
constexpr std::uint32_t flg_heap_enable_tag_by_dll = 0x8000;
constexpr std::uint32_t flg_heap_enable_tail_check = 0x10;
constexpr std::uint32_t flg_heap_validate_all = 0x80;
constexpr std::uint32_t flg_enable_kdebug_symbol_load = 0x040000;
constexpr std::uint32_t flg_enable_handle_type_tagging = 0x01000000;
constexpr std::uint32_t flg_heap_page_allocs = 0x02000000;
constexpr std::uint32_t flg_pool_enable_tagging = 0x0400;
constexpr std::uint32_t flg_enable_system_crit_breaks = 0x100000;
constexpr std::uint32_t flg_maintain_object_typelist = 0x4000;
constexpr std::uint32_t flg_monitor_silent_process_exit = 0x200;
constexpr std::uint32_t flg_show_ldr_snaps = 0x02;
constexpr std::uint32_t flg_stop_on_exception = 0x01;
constexpr std::uint32_t flg_stop_on_hung_gui = 0x08;
constexpr std::uint32_t flg_stop_on_unhandled_exception = 0x20000000;
} //namespace gflags

namespace heap_flags
{
constexpr std::uint32_t heap_create_enable_execute = 0x00040000;
constexpr std::uint32_t heap_generate_exceptions = 0x00000004;
constexpr std::uint32_t heap_no_serialize = 0x00000001;
} //namespace heap_flags

namespace dependent_load_flags
{
constexpr std::uint16_t load_library_search_application_dir = 0x0200u;
constexpr std::uint16_t load_library_search_default_dirs = 0x1000u;
constexpr std::uint16_t load_library_search_dll_load_dir = 0x0100u;
constexpr std::uint16_t load_library_search_system32 = 0x0800u;
constexpr std::uint16_t load_library_search_user_dirs = 0x0400u;
} //namespace dependent_load_flags

//XXX: undocumented
struct image_volatile_metadata
{
	std::uint32_t size;
	std::uint32_t version;
	std::uint32_t volatile_access_table;
	std::uint32_t volatile_access_table_size;
	std::uint32_t volatile_info_range_table;
	std::uint32_t volatile_info_range_table_size;
};

struct range_table_entry
{
	std::uint32_t rva;
	std::uint32_t size;
};

} //namespace pe_bliss::detail::load_config
