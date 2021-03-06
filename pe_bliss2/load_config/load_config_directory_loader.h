#pragma once

#include <optional>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/load_config/load_config_directory.h"

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::load_config
{

enum class load_config_directory_loader_errc
{
	invalid_dynamic_relocation_table_section_index = 1,
	invalid_dynamic_relocation_table_section_offset,
	unknown_dynamic_relocation_table_version,
	invalid_lock_prefix_table,
	invalid_safeseh_handler_table,
	invalid_cf_function_table,
	invalid_cf_export_suppression_table,
	invalid_cf_longjump_table,
	unknown_chpe_metadata_type,
	invalid_chpe_metadata,
	invalid_chpe_range_entries,
	invalid_dynamic_relocation_table,
	invalid_dynamic_relocation_table_v2_size,
	invalid_dynamic_relocation_table_v1_size,
	invalid_dynamic_relocation_header_size,
	invalid_dynamic_relocation_fixup_info_size,
	invalid_dynamic_relocation_prologue,
	invalid_dynamic_relocation_epilogue,
	unknown_dynamic_relocation_symbol,
	invalid_dynamic_relocation_block_size,
	unaligned_dynamic_relocation_block,
	unknown_arm64x_relocation_type,
	invalid_arm64x_dynamic_relocation_copy_data_size,
	invalid_arm64x_dynamic_relocation_add_delta_size,
	invalid_base_relocation_size,
	invalid_dynamic_relocation_size,
	invalid_cf_guard_table_size,
	unsorted_cf_guard_table,
	invalid_dynamic_relocation_epilogue_size,
	invalid_dynamic_relocation_epilogue_branch_descriptor_size,
	invalid_dynamic_relocation_epilogue_branch_descriptors,
	invalid_dynamic_relocation_epilogue_branch_descriptor_bit_map,
	invalid_enclave_config,
	invalid_enclave_config_extra_data,
	invalid_enclave_import_extra_data,
	invalid_enclave_import_name,
	invalid_volatile_metadata,
	unaligned_volatile_metadata_access_rva_table_size,
	unaligned_volatile_metadata_range_table_size,
	invalid_volatile_metadata_access_rva_table,
	invalid_volatile_metadata_range_table,
	invalid_ehcont_targets,
	unsorted_ehcont_targets,
	invalid_xfg_type_based_hash_rva
};

std::error_code make_error_code(load_config_directory_loader_errc) noexcept;

struct loader_options
{
	bool include_headers = true;
	bool allow_virtual_data = false;
	bool load_lock_prefix_table = true;
	bool load_safeseh_handler_table = true;
	bool load_cf_guard_function_table = true;
	bool load_cf_guard_longjump_table = true;
	bool load_cf_guard_export_suppression_table = true;
	bool load_chpe_metadata = true;
	bool load_dynamic_relocation_table = true;
	bool load_enclave_config = true;
	bool load_volatile_metadata = true;
	bool load_ehcont_targets = true;
	bool load_xfg_type_based_hashes = true;
};

std::optional<load_config_directory_details> load(const image::image& instance,
	const loader_options& options);

} //namespace pe_bliss::load_config

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::load_config::load_config_directory_loader_errc> : true_type {};
} //namespace std
