#include "pe_bliss2/load_config/load_config_directory_loader.h"

#include <bit>
#include <cassert>
#include <climits>
#include <iterator>
#include <limits>
#include <optional>
#include <system_error>
#include <variant>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/core/optional_header.h"
#include "pe_bliss2/detail/load_config/image_load_config_directory.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/byte_array_from_va.h"
#include "pe_bliss2/image/byte_vector_from_va.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/image/string_from_va.h"
#include "pe_bliss2/image/struct_from_va.h"
#include "pe_bliss2/packed_struct.h"
#include "utilities/generic_error.h"
#include "utilities/math.h"
#include "utilities/safe_uint.h"

namespace
{

struct load_config_directory_loader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "load_config_directory_loader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::load_config::load_config_directory_loader_errc;
		switch (static_cast<pe_bliss::load_config::load_config_directory_loader_errc>(ev))
		{
		case invalid_dynamic_relocation_table_section_index:
			return "Invalid dynamic relocation table section index";
		case invalid_dynamic_relocation_table_section_offset:
			return "Invalid dynamic relocation table section offset";
		case unknown_dynamic_relocation_table_version:
			return "Unknown dynamic relocation table version";
		case invalid_lock_prefix_table:
			return "Invalid lock prefix table";
		case invalid_safeseh_handler_table:
			return "Invalid SafeSEH handler table";
		case invalid_cf_function_table:
			return "Invalid CF function table";
		case invalid_cf_export_suppression_table:
			return "Invalid CF guard export suppression table";
		case invalid_cf_longjump_table:
			return "Invalid CF guard longjump table";
		case unknown_chpe_metadata_type:
			return "Unknown hybrid PE metadata type";
		case invalid_chpe_metadata:
			return "Invalid hybrid PE metadata";
		case invalid_chpe_range_entries:
			return "Invalid hybrid PE range entries";
		case invalid_dynamic_relocation_table:
			return "Invalid dynamic relocation table";
		case invalid_dynamic_relocation_table_v2_size:
			return "Invalid dynamic relocation table V2 size";
		case invalid_dynamic_relocation_table_v1_size:
			return "Invalid dynamic relocation table V1 size";
		case invalid_dynamic_relocation_header_size:
			return "Invalid dynamic relocation header size";
		case invalid_dynamic_relocation_fixup_info_size:
			return "Invalid dynamic relocation fixup info size";
		case invalid_dynamic_relocation_prologue:
			return "Invalid dynamic relocation prologue";
		case invalid_dynamic_relocation_epilogue:
			return "Invalid dynamic relocation epilogue";
		case unknown_dynamic_relocation_symbol:
			return "Unknown dynamic relocation symbol";
		case invalid_dynamic_relocation_block_size:
			return "Invalid dynamic relocation block size";
		case unaligned_dynamic_relocation_block:
			return "Unaligned dynamic relocation block";
		case unknown_arm64x_relocation_type:
			return "Unknown ARM64X relocation type";
		case invalid_arm64x_dynamic_relocation_copy_data_size:
			return "Invalid ARM64X dynamic relocation (copy data) size";
		case invalid_arm64x_dynamic_relocation_add_delta_size:
			return "Invalid ARM64X dynamic relocation (add delta) size";
		case invalid_base_relocation_size:
			return "Invalid base relocation size";
		case invalid_dynamic_relocation_size:
			return "Invalid dynamic relocation size";
		case invalid_cf_guard_table_size:
			return "Invalid CF guard table size";
		case unsorted_cf_guard_table:
			return "Unsorted CF guard table";
		case invalid_dynamic_relocation_epilogue_size:
			return "Invalid dynamic relocation epilogue size";
		case invalid_dynamic_relocation_epilogue_branch_descriptor_size:
			return "Invalid dynamic relocation epilogue branch descriptor size";
		case invalid_dynamic_relocation_epilogue_branch_descriptors:
			return "Invalid dynamic relocation epilogue branch descriptors";
		case invalid_dynamic_relocation_epilogue_branch_descriptor_bit_map:
			return "Invalid dynamic relocation epilogue branch descriptor bit map";
		case invalid_enclave_config:
			return "Invalid enclave configuration";
		case invalid_enclave_config_extra_data:
			return "Invalid enclave configuration extra data";
		case invalid_enclave_import_extra_data:
			return "Invalid enclave import extra data";
		case invalid_enclave_import_name:
			return "Invalid enclave import name";
		case invalid_volatile_metadata:
			return "Invalid volatile metadata";
		case unaligned_volatile_metadata_access_rva_table_size:
			return "Unaligned volatile metadata access table size";
		case unaligned_volatile_metadata_range_table_size:
			return "Unaligned volatile metadata range table size";
		case invalid_volatile_metadata_access_rva_table:
			return "Invalid volatile metadata access RVA table";
		case invalid_volatile_metadata_range_table:
			return "Invalid volatile metadata range table";
		case invalid_ehcont_targets:
			return "Invalid exception handling continuation targets";
		case unsorted_ehcont_targets:
			return "Unsorted exception handling continuation target RVAs";
		case invalid_xfg_type_based_hash_rva:
			return "Invalid XFG type-based hash RVA";
		case invalid_func_override_size:
			return "Invalid func override block size";
		case invalid_func_override_rvas_size:
			return "Invalid func override RVAs size";
		case invalid_bdd_info_size:
			return "Invalid func override BDD info size";
		case invalid_load_config_directory:
			return "Invalid load configuration directory";
		case invalid_guard_memcpy_function_pointer_va:
			return "Invalid guard memcpy function pointer virtual address";
		case invalid_cast_guard_os_determined_failure_mode_va:
			return "Invalid CastGuard OS determined failure mode virtual address";
		case invalid_guard_xfg_check_function_pointer_va:
			return "Invalid extended guard check function pointer virtual address";
		case invalid_guard_xfg_dispatch_function_pointer_va:
			return "Invalid extended guard dispatch function pointer virtual address";
		case invalid_guard_xfg_table_dispatch_function_pointer_va:
			return "Invalid extended guard table dispatch function pointer virtual address";
		case invalid_enclave_import_array:
			return "Invalid enclave import array";
		case invalid_dynamic_relocation_entry:
			return "Invalid dynamic relocation entry";
		case invalid_base_relocation:
			return "Invalid base relocation entry";
		case invalid_dynamic_relocation_block:
			return "Invalid dynamic relocation block";
		case invalid_func_override_fixup:
			return "Invalid func override fixup entry";
		case invalid_bdd_info_entry:
			return "Invalid func override BDD info entry";
		case invalid_bdd_dynamic_relocations:
			return "Invalid func override BDD info dynamic relocations";
		case invalid_func_override_dynamic_relocation:
			return "Invalid func override dynamic relocation";
		case invalid_func_override_rvas:
			return "Invalid func override RVAs";
		case invalid_arm64x_relocation_entry:
			return "Invalid ARM64X relocation entry";
		case invalid_arm64x_dynamic_relocation_copy_data_data:
			return "Invalid ARM64X dynamic relocation (copy data) data";
		case invalid_arm64x_dynamic_relocation_add_delta_entry:
			return "Invalid ARM64X dynamic relocation (add delta) entry";
		case invalid_security_cookie_va:
			return "Invalid security cookie virtual address";
		case invalid_cf_guard_table_function_count:
			return "Invalid CF guard table function count";
		case invalid_guard_export_suppression_table_size:
			return "Invalid CF guard export suppression table size";
		case invalid_guard_export_suppression_table_function_count:
			return "Invalid CF guard export suppression table function count";
		case unsorted_guard_export_suppression_table:
			return "Unsorted CF guard export suppression table";
		case invalid_guard_longjump_table_size:
			return "Invalid CF guard longjump table size";
		case invalid_guard_longjump_table_function_count:
			return "Invalid CF guard longjump table function count";
		case unsorted_guard_longjump_table:
			return "Unsorted CF guard longjump table";
		case invalid_guard_cf_check_function_va:
			return "Invalid CF guard check function virtual address";
		case invalid_guard_cf_dispatch_function_va:
			return "Invalid CF guard dispatch function virtual address";
		case invalid_chpe_range_entry_count:
			return "Invalid CHPE range entry count";
		case invalid_chpe_entry_address_or_size:
			return "Invalid CHPE entry address or size";
		case unknown_bdd_info_entry_version:
			return "Unknown BDD info entry version";
		case invalid_volatile_metadata_access_rva_table_entry_count:
			return "Invalid volatile metadata access RVA table entry count";
		case invalid_volatile_metadata_range_table_entry_count:
			return "Invalid volatile metadata range table entry count";
		case invalid_ehcont_target_rvas:
			return "Invalid EH continuation targets RVAs";
		case invalid_ehcont_targets_count:
			return "Invalid EH continuation targets count";
		default:
			return {};
		}
	}
};

const load_config_directory_loader_error_category load_config_directory_loader_error_category_instance;

using namespace pe_bliss;
using namespace pe_bliss::load_config;

template<typename Va, typename Directory>
void validate_va(const image::image& instance, Va va,
	bool include_headers, bool allow_virtual_data, Directory& directory,
	load_config_directory_loader_errc errc)
{
	if (!va)
		return;

	try
	{
		[[maybe_unused]] auto first_byte = struct_from_va<std::uint8_t>(
			instance, va, include_headers, allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		directory.add_error(errc);
	}
}

template<typename Directory>
void load_lock_prefix_table(const image::image& instance,
	const loader_options& options, Directory& directory) try
{
	if (!options.load_lock_prefix_table)
		return;

	utilities::safe_uint lock_prefix_table_va = directory.get_descriptor()
		->base.lock_prefix_table;
	if (!lock_prefix_table_va)
		return;

	auto& table = directory.get_lock_prefix_table().emplace().get_prefix_va_list();
	using va_type = typename Directory::lock_prefix_table_type::value_type::pointer_type;
	packed_struct<va_type> lock_prefix_va;
	while (struct_from_va(instance, lock_prefix_table_va.value(), lock_prefix_va,
		options.include_headers, options.allow_virtual_data).get())
	{
		table.emplace_back(lock_prefix_va);
		lock_prefix_table_va += lock_prefix_va.packed_size;
	}
}
catch (const std::system_error&)
{
	directory.add_error(
		load_config_directory_loader_errc::invalid_lock_prefix_table);
}

void load_safeseh_handler_table(const image::image& instance,
	const loader_options& options,
	load_config_directory_details::underlying_type32& directory)
{
	if (!options.load_safeseh_handler_table)
		return;

	if (instance.get_optional_header().get_dll_characteristics()
		& core::optional_header::dll_characteristics::no_seh)
	{
		return;
	}

	auto safeseh_handler_table_va = directory.get_descriptor()
		->structured_exceptions.se_handler_table;
	if (!safeseh_handler_table_va)
		return;

	auto count = directory.get_descriptor()->structured_exceptions.se_handler_count;
	auto handlers_size = static_cast<std::uint64_t>(count) * sizeof(rva_type);
	if (handlers_size > (std::numeric_limits<rva_type>::max)()
		|| !utilities::math::is_sum_safe(safeseh_handler_table_va,
			static_cast<rva_type>(handlers_size)))
	{
		directory.add_error(
			load_config_directory_loader_errc::invalid_safeseh_handler_table);
		return;
	}

	if (count > options.max_safeseh_handler_count)
	{
		count = options.max_safeseh_handler_count;
		directory.add_error(
			load_config_directory_loader_errc::invalid_safeseh_handler_table);
	}

	auto& table = directory.get_safeseh_handler_table().emplace().get_handler_list();
	table.reserve(count);
	while (count--)
	{
		try
		{
			struct_from_va(instance, safeseh_handler_table_va, table.emplace_back(),
				options.include_headers, options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			table.pop_back();
			directory.add_error(
				load_config_directory_loader_errc::invalid_safeseh_handler_table);
			return;
		}
		safeseh_handler_table_va += sizeof(rva_type);
	}
}

constexpr void load_safeseh_handler_table(
	const image::image&, const loader_options&,
	load_config_directory_details::underlying_type64&) noexcept
{
	//No SafeSEH on PE+
}

bool has_cf_guard(const image::image& instance) noexcept
{
	return static_cast<bool>(instance.get_optional_header().get_dll_characteristics()
		& core::optional_header::dll_characteristics::guard_cf);
}

template<typename GuardFunction, typename Directory,
	typename Va, typename Table>
void read_cf_guard_rva_table(const image::image& instance,
	const loader_options& options, Directory& directory,
	std::uint64_t max_functions,
	Va table_va, Va function_count, Table& optional_table,
	load_config_directory_loader_errc invalid_table_size_error,
	load_config_directory_loader_errc invalid_function_count_error,
	load_config_directory_loader_errc unsorted_table_error)
{
	if (!table_va)
		return;

	auto stride = directory.get_guard_cf_function_table_stride();

	auto entry_size = sizeof(rva_type) + stride;
	auto handlers_size = static_cast<std::uint64_t>(function_count) * entry_size;
	//Check multiplication overflow
	if (function_count && handlers_size / function_count != entry_size) {
		directory.add_error(invalid_table_size_error);
		return;
	}
	if (handlers_size > (std::numeric_limits<Va>::max)()
		|| !utilities::math::is_sum_safe(table_va, static_cast<Va>(handlers_size)))
	{
		directory.add_error(invalid_table_size_error);
		return;
	}

	if (function_count > max_functions)
	{
		function_count = static_cast<Va>(max_functions);
		directory.add_error(invalid_function_count_error);
	}

	auto& table = optional_table.emplace();
	rva_type prev_rva{};
	bool is_sorted = true;
	table.reserve(static_cast<std::size_t>(function_count));
	while (function_count--)
	{
		GuardFunction& func = table.emplace_back();
		try
		{
			struct_from_va(instance, table_va, func.get_rva(),
				options.include_headers, options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			table.pop_back();
			throw;
		}

		table_va += std::remove_cvref_t<decltype(func.get_rva())>::packed_size;
		if (stride)
		{
			byte_array_from_va(instance, table_va, stride, func.get_additional_data(),
				options.include_headers, options.allow_virtual_data);
			if (!utilities::math::add_if_safe(table_va,
				static_cast<Va>(func.get_additional_data().data_size())))
			{
				throw pe_error(utilities::generic_errc::integer_overflow);
			}
		}

		if (is_sorted)
		{
			is_sorted = func.get_rva().get() >= prev_rva;
			prev_rva = func.get_rva().get();
		}
	}

	if (!is_sorted)
		directory.add_error(unsorted_table_error);
}

template<typename Directory>
void load_cf_guard(const image::image& instance,
	const loader_options& options, Directory& directory)
{
	if (!options.load_cf_guard_function_table)
		return;

	if (!has_cf_guard(instance))
		return;

	if (!(directory.get_guard_flags() & guard_flags::cf_instrumented))
		return;

	validate_va(instance, directory.get_descriptor()->cf_guard.guard_cf_check_function_pointer,
		options.include_headers, options.allow_virtual_data, directory,
		load_config_directory_loader_errc::invalid_guard_cf_check_function_va);
	validate_va(instance, directory.get_descriptor()->cf_guard.guard_cf_dispatch_function_pointer,
		options.include_headers, options.allow_virtual_data, directory,
		load_config_directory_loader_errc::invalid_guard_cf_dispatch_function_va);

	if (!(directory.get_guard_flags() & guard_flags::cf_function_table_present))
		return;

	try
	{
		read_cf_guard_rva_table<guard_function_details>(instance, options, directory,
			options.max_cf_function_table_functions,
			directory.get_descriptor()->cf_guard.guard_cf_function_table,
			directory.get_descriptor()->cf_guard.guard_cf_function_count,
			directory.get_guard_cf_function_table(),
			load_config_directory_loader_errc::invalid_cf_guard_table_size,
			load_config_directory_loader_errc::invalid_cf_guard_table_function_count,
			load_config_directory_loader_errc::unsorted_cf_guard_table);
	}
	catch (const std::system_error&)
	{
		directory.add_error(
			load_config_directory_loader_errc::invalid_cf_function_table);
	}

	if (!options.load_xfg_type_based_hashes
		|| !directory.get_guard_cf_function_table())
	{
		return;
	}

	if (!(directory.get_guard_flags() & guard_flags::xfg_enabled))
		return;

	for (auto& entry : *directory.get_guard_cf_function_table())
	{
		if (!(entry.get_flags() & gfids_flags::fid_xfg))
			continue;

		auto func_rva = entry.get_rva().get();
		if (func_rva < guard_function_details::type_based_hash_type::packed_size)
		{
			entry.add_error(
				load_config_directory_loader_errc::invalid_xfg_type_based_hash_rva);
			continue;
		}

		try
		{
			struct_from_rva(instance,
				func_rva - guard_function_details::type_based_hash_type::packed_size,
				entry.get_type_based_hash().emplace(),
				options.include_headers, options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			entry.get_type_based_hash().reset();
			entry.add_error(
				load_config_directory_loader_errc::invalid_xfg_type_based_hash_rva);
		}
	}
}

template<typename Directory>
void load_cf_guard_export_suppression_table(const image::image& instance,
	const loader_options& options, Directory& directory) try
{
	if (!options.load_cf_guard_export_suppression_table)
		return;

	if (!has_cf_guard(instance))
		return;

	static constexpr auto flags = guard_flags::cf_instrumented
		| guard_flags::cf_export_suppression_info_present;
	if ((directory.get_guard_flags() & flags) != flags)
		return;

	read_cf_guard_rva_table<guard_function_common>(instance, options, directory,
		options.max_guard_export_suppression_table_functions,
		directory.get_descriptor()->cf_guard_ex.guard_address_taken_iat_entry_table,
		directory.get_descriptor()->cf_guard_ex.guard_address_taken_iat_entry_count,
		directory.get_guard_address_taken_iat_entry_table(),
		load_config_directory_loader_errc::invalid_guard_export_suppression_table_size,
		load_config_directory_loader_errc::invalid_guard_export_suppression_table_function_count,
		load_config_directory_loader_errc::unsorted_guard_export_suppression_table);
}
catch (const std::system_error&)
{
	directory.add_error(
		load_config_directory_loader_errc::invalid_cf_export_suppression_table);
}

template<typename Directory>
void load_cf_guard_longjump_table(const image::image& instance,
	const loader_options& options, Directory& directory) try
{
	if (!options.load_cf_guard_longjump_table)
		return;

	if (!has_cf_guard(instance))
		return;

	static constexpr auto flags = guard_flags::cf_instrumented
		| guard_flags::cf_longjump_table_present;
	if ((directory.get_guard_flags() & flags) != flags)
		return;

	read_cf_guard_rva_table<guard_function_common>(instance, options, directory,
		options.max_guard_longjump_table_functions,
		directory.get_descriptor()->cf_guard_ex.guard_long_jump_target_table,
		directory.get_descriptor()->cf_guard_ex.guard_long_jump_target_count,
		directory.get_guard_long_jump_target_table(),
		load_config_directory_loader_errc::invalid_guard_longjump_table_size,
		load_config_directory_loader_errc::invalid_guard_longjump_table_function_count,
		load_config_directory_loader_errc::unsorted_guard_longjump_table);
}
catch (const std::system_error&)
{
	directory.add_error(
		load_config_directory_loader_errc::invalid_cf_longjump_table);
}

template<typename Metadata>
void load_chpe_range_entries(const image::image& instance,
	const loader_options& options, Metadata& metadata)
{
	auto& metadata_descriptor = metadata.get_metadata();
	auto entry_count = metadata_descriptor->cphe_code_address_range_count;
	auto entry_rva = metadata_descriptor->cphe_code_address_range_offset;

	auto entries_size = static_cast<std::uint64_t>(entry_count)
		* Metadata::range_entry_list_type::value_type::range_entry_type::packed_size;
	if (entries_size > (std::numeric_limits<rva_type>::max)()
		|| !utilities::math::is_sum_safe(entry_rva,
			static_cast<rva_type>(entries_size)))
	{
		metadata.add_error(
			load_config_directory_loader_errc::invalid_chpe_range_entries);
		return;
	}

	auto& entry_list = metadata.get_range_entries();
	if (entry_count > options.max_cphe_code_address_range_count)
	{
		entry_count = options.max_cphe_code_address_range_count;
		metadata.add_error(
			load_config_directory_loader_errc::invalid_chpe_range_entry_count);
	}

	entry_list.reserve(entry_count);
	while (entry_count--)
	{
		auto& entry = entry_list.emplace_back();
		try
		{
			entry_rva += static_cast<rva_type>(
				struct_from_rva(instance, entry_rva, entry.get_entry(),
					options.include_headers, options.allow_virtual_data).packed_size);
		}
		catch (const std::system_error&)
		{
			entry_list.pop_back();
			metadata.add_error(
				load_config_directory_loader_errc::invalid_chpe_range_entries);
			break;
		}

		try
		{
			auto buf = section_data_from_rva(instance, entry.get_rva(),
				options.include_headers, options.allow_virtual_data);
			if (buf->size() < entry.get_entry()->length)
				throw pe_error(utilities::generic_errc::buffer_overrun);
		}
		catch (const std::system_error&)
		{
			entry.add_error(
				load_config_directory_loader_errc::invalid_chpe_entry_address_or_size);
		}
	}
}

template<typename Va>
void load_chpe_metadata(const image::image& instance,
	const loader_options& options, utilities::safe_uint<Va> metadata_va,
	chpe_arm64x_metadata_details& metadata)
{
	metadata_va += struct_from_va(instance, metadata_va.value(), metadata.get_version(),
		options.include_headers, options.allow_virtual_data).packed_size;
	struct_from_va(instance, metadata_va.value(), metadata.get_metadata(),
		options.include_headers, options.allow_virtual_data);
	load_chpe_range_entries(instance, options, metadata);
}

template<typename Va>
void load_chpe_metadata(const image::image& instance,
	const loader_options& options, utilities::safe_uint<Va> metadata_va,
	chpe_x86_metadata_details& metadata)
{
	metadata_va += struct_from_va(instance, metadata_va.value(), metadata.get_version(),
		options.include_headers, options.allow_virtual_data).packed_size;

	auto buf = section_data_from_va(instance, metadata_va.value(),
		options.include_headers, options.allow_virtual_data);
	auto& metadata_descriptor = metadata.get_metadata();
	buffers::input_buffer_stateful_wrapper_ref wrapper(*buf);
	metadata_descriptor.deserialize_until(wrapper,
		metadata.get_metadata_size(), options.allow_virtual_data);

	load_chpe_range_entries(instance, options, metadata);
}

template<typename Directory>
void load_chpe_metadata(const image::image& instance,
	const loader_options& options, Directory& directory) try
{
	if (!options.load_chpe_metadata)
		return;

	utilities::safe_uint metadata_va = directory.get_descriptor()
		->hybrid_pe.chpe_metadata_pointer;
	if (!metadata_va)
		return;

	auto machine = instance.get_file_header().get_machine_type();
	if (machine == core::file_header::machine_type::arm64
		|| machine == core::file_header::machine_type::amd64)
	{
		load_chpe_metadata(instance, options, metadata_va,
			directory.get_chpe_metadata().template emplace<chpe_arm64x_metadata_details>());
	}
	else if (machine == core::file_header::machine_type::chpe_x86)
	{
		load_chpe_metadata(instance, options, metadata_va,
			directory.get_chpe_metadata().template emplace<chpe_x86_metadata_details>());
	}
	else
	{
		directory.add_error(load_config_directory_loader_errc::unknown_chpe_metadata_type);
	}
}
catch (const std::system_error&)
{
	directory.add_error(load_config_directory_loader_errc::invalid_chpe_metadata);
}

constexpr void load_dynamic_relocation_table(
	const image::image&, const loader_options&,
	load_config_directory_details::underlying_type32&) noexcept
{
	//No DVRT on PE32
}

template<typename DynamicRelocationList>
bool load_dynamic_relocation_struct(const image::image& instance,
	const loader_options& options,
	rva_type last_rva, rva_type& current_rva, DynamicRelocationList& list)
{
	if (last_rva == current_rva)
		return false; //No more elements

	auto& relocation_list = list.emplace_back();
	if (last_rva - current_rva
		< DynamicRelocationList::value_type::dynamic_relocation_type::packed_size)
	{
		relocation_list.add_error(
			load_config_directory_loader_errc::invalid_dynamic_relocation_size);
		return false;
	}

	try
	{
		auto packed_size = static_cast<rva_type>(
			struct_from_rva(instance, current_rva,
				relocation_list.get_dynamic_relocation(),
				options.include_headers, options.allow_virtual_data).packed_size);
		if (!utilities::math::add_if_safe(current_rva, packed_size))
			throw pe_error(utilities::generic_errc::integer_overflow);
	}
	catch (const std::system_error&)
	{
		relocation_list.add_error(
			load_config_directory_loader_errc::invalid_dynamic_relocation_entry);
		return false;
	}
	return true;
}

using import_control_transfer_dynamic_relocation_list_type
	= dynamic_relocation_table_v1_details<std::uint64_t>::import_control_transfer_dynamic_relocation_list_type;
using indir_control_transfer_dynamic_relocation_list_type
	= dynamic_relocation_table_v1_details<std::uint64_t>::indir_control_transfer_dynamic_relocation_list_type;
using switchtable_branch_dynamic_relocation_list_type
	= dynamic_relocation_table_v1_details<std::uint64_t>::switchtable_branch_dynamic_relocation_list_type;
using arm64x_dynamic_relocation_list_type
	= dynamic_relocation_table_v1_details<std::uint64_t>::arm64x_dynamic_relocation_list_type;
using function_override_dynamic_relocation_list_type
	= dynamic_relocation_table_v1_details<std::uint64_t>::function_override_dynamic_relocation_list_type;

template<typename DynamicRelocationList>
bool load_base_relocation(const image::image& instance, const loader_options& options,
	rva_type current_rva, rva_type last_base_reloc_rva, DynamicRelocationList& list)
{
	if (last_base_reloc_rva == current_rva)
		return false; //No more elements
	
	auto& fixups = list.emplace_back();
	if (last_base_reloc_rva - current_rva
		< DynamicRelocationList::value_type::base_relocation_type::packed_size)
	{
		fixups.add_error(load_config_directory_loader_errc::invalid_base_relocation_size);
		return false;
	}

	try
	{
		struct_from_rva(instance, current_rva, fixups.get_base_relocation(),
			options.include_headers, options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		fixups.add_error(load_config_directory_loader_errc::invalid_base_relocation);
		return false;
	}

	if (fixups.get_base_relocation()->size_of_block > last_base_reloc_rva - current_rva)
	{
		fixups.add_error(load_config_directory_loader_errc::invalid_base_relocation_size);
		return false;
	}

	if (fixups.get_base_relocation()->size_of_block
		< DynamicRelocationList::value_type::base_relocation_type::packed_size)
	{
		fixups.add_error(load_config_directory_loader_errc::invalid_base_relocation_size);
		return false;
	}

	if (current_rva % sizeof(rva_type))
	{
		fixups.add_error(
			load_config_directory_loader_errc::unaligned_dynamic_relocation_block);
	}

	return true;
}

template<typename Fixups>
void check_fixup_end_block_rva(rva_type current_rva, rva_type block_end_rva, Fixups& fixups)
{
	if (block_end_rva != current_rva)
	{
		fixups.add_error(
			load_config_directory_loader_errc::invalid_dynamic_relocation_block_size);
	}
}

template<typename DynamicRelocationList>
void load_relocations_no_extra_data(const image::image& instance, const loader_options& options,
	rva_type current_rva, rva_type last_base_reloc_rva, DynamicRelocationList& list)
{
	while (load_base_relocation(instance, options, current_rva, last_base_reloc_rva, list))
	{
		auto& fixups = list.back();
		//size_of_block is validated by load_base_relocation() call
		auto block_end_rva = current_rva + fixups.get_base_relocation()->size_of_block;
		current_rva += DynamicRelocationList::value_type::base_relocation_type::packed_size;

		auto& fixup_list = fixups.get_fixups();
		bool first_element = true;
		while (block_end_rva - current_rva
			>= DynamicRelocationList::value_type::symbol_type::relocation_type::packed_size)
		{
			try
			{
				current_rva += static_cast<rva_type>(
					struct_from_rva(instance, current_rva, fixup_list.emplace_back().get_relocation(),
						options.include_headers, options.allow_virtual_data).packed_size);
			}
			catch (const std::system_error&)
			{
				fixup_list.pop_back();
				fixups.add_error(
					load_config_directory_loader_errc::invalid_dynamic_relocation_block);
				break;
			}

			if (!first_element)
			{
				if constexpr (std::is_scalar_v<
					typename DynamicRelocationList
						::value_type::symbol_type::relocation_type::value_type>)
				{
					if (!fixup_list.back().get_relocation().get())
						fixup_list.pop_back(); //Zero element for alignment
				}
				else
				{
					if (!fixup_list.back().get_relocation()->metadata)
						fixup_list.pop_back(); //Zero element for alignment
				}
			}

			first_element = false;
		}

		check_fixup_end_block_rva(current_rva, block_end_rva, fixups);
		current_rva = block_end_rva;
	}
}

void load_arm64x_relocations(const image::image& instance, const loader_options& options,
	rva_type current_rva, rva_type last_base_reloc_rva,
	arm64x_dynamic_relocation_list_type& list)
{
	while (load_base_relocation(
		instance, options, current_rva, last_base_reloc_rva, list))
	{
		auto& fixups = list.back();
		//size_of_block is validated by load_base_relocation() call
		auto block_end_rva = current_rva + fixups.get_base_relocation()->size_of_block;
		current_rva
			+= std::remove_cvref_t<decltype(fixups.get_base_relocation())>::packed_size;

		bool first_element = true;
		while (block_end_rva - current_rva
			>= arm64x_dynamic_relocation_base::relocation_type::packed_size)
		{
			arm64x_dynamic_relocation_base relocation_base;
			try
			{
				current_rva += static_cast<rva_type>(
					struct_from_rva(instance, current_rva, relocation_base.get_relocation(),
						options.include_headers, options.allow_virtual_data).packed_size);
			}
			catch (const std::system_error&)
			{
				fixups.add_error(
					load_config_directory_loader_errc::invalid_arm64x_relocation_entry);
				break;
			}

			//TODO: may loop for long time if allow_virtual_data=true and all
			//remainind relocations are virtual
			if (!first_element && !relocation_base.get_relocation()->metadata)
				continue; //Zero element for alignment

			first_element = false;

			using enum arm64x_dynamic_relocation_base::type;
			switch (relocation_base.get_type())
			{
			case zero_fill:
				fixups.get_fixups().emplace_back() = arm64x_dynamic_relocation_zero_fill{
					relocation_base
				};
				break;
			case copy_data:
				{
					auto& element = std::get<arm64x_dynamic_relocation_copy_data_details>
						(fixups.get_fixups().emplace_back(
							std::in_place_type<arm64x_dynamic_relocation_copy_data_details>));
					element.get_relocation() = relocation_base.get_relocation();
					if (block_end_rva - current_rva < element.get_size())
					{
						element.add_error(
							load_config_directory_loader_errc::invalid_arm64x_dynamic_relocation_copy_data_size);
					}
					else
					{
						try
						{
							byte_array_from_rva(instance, current_rva, element.get_size(),
								element.get_data(), options.include_headers, options.allow_virtual_data);
						}
						catch (const std::system_error&)
						{
							element.add_error(
								load_config_directory_loader_errc::invalid_arm64x_dynamic_relocation_copy_data_data);
							break;
						}
						current_rva += element.get_size();
					}
				}
				break;
			case add_delta:
				{
					auto& element = std::get<arm64x_dynamic_relocation_add_delta_details>
						(fixups.get_fixups().emplace_back(
							std::in_place_type<arm64x_dynamic_relocation_add_delta_details>));
					element.get_relocation() = relocation_base.get_relocation();
					if (block_end_rva - current_rva
						< std::remove_cvref_t<decltype(element.get_value())>::packed_size)
					{
						element.add_error(
							load_config_directory_loader_errc::invalid_arm64x_dynamic_relocation_add_delta_size);
					}
					else
					{
						try
						{
							current_rva += static_cast<rva_type>(
								struct_from_rva(instance, current_rva, element.get_value(),
									options.include_headers, options.allow_virtual_data).packed_size);
						}
						catch (const std::system_error&)
						{
							element.add_error(
								load_config_directory_loader_errc::invalid_arm64x_dynamic_relocation_add_delta_entry);
							break;
						}
					}
				}
				break;
			default:
				fixups.add_error(load_config_directory_loader_errc::unknown_arm64x_relocation_type);
				current_rva = block_end_rva;
				break;
			}
		}

		check_fixup_end_block_rva(current_rva, block_end_rva, fixups);
		current_rva = block_end_rva;
	}
}

void load_bdd_info(const image::image& instance,
	const loader_options& options,
	std::uint32_t bdd_offset, rva_type bdd_rva, rva_type block_end_rva,
	bdd_info<error_list>& bdd)
{
	rva_type descriptor_rva = bdd_rva;
	if (!utilities::math::add_if_safe(descriptor_rva, bdd_offset))
	{
		bdd.add_error(load_config_directory_loader_errc::invalid_bdd_info_entry);
		return;
	}

	rva_type descriptor_end_rva = bdd_info<error_list>::descriptor_type::packed_size;
	if (!utilities::math::add_if_safe(descriptor_end_rva, descriptor_rva)
		|| descriptor_end_rva > block_end_rva)
	{
		bdd.add_error(load_config_directory_loader_errc::invalid_bdd_info_entry);
		return;
	}

	try
	{
		struct_from_rva(instance, descriptor_rva, bdd.get_descriptor(),
			options.include_headers, options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		bdd.add_error(load_config_directory_loader_errc::invalid_bdd_info_entry);
		return;
	}

	if (bdd.get_descriptor()->version > 1u)
	{
		bdd.add_error(load_config_directory_loader_errc::unknown_bdd_info_entry_version);
		return;
	}

	auto remaining_size = (std::min)(block_end_rva - descriptor_end_rva,
		bdd.get_descriptor()->bdd_size);
	while (remaining_size >= bdd_info<error_list>::dynamic_relocation_type::packed_size)
	{
		auto& reloc = bdd.get_relocations().emplace_back();
		try
		{
			struct_from_rva(instance, descriptor_end_rva, reloc,
				options.include_headers, options.allow_virtual_data);
			descriptor_end_rva += bdd_info<error_list>::dynamic_relocation_type::packed_size;
			remaining_size -= bdd_info<error_list>::dynamic_relocation_type::packed_size;
		}
		catch (const std::system_error&)
		{
			bdd.get_relocations().pop_back();
			bdd.add_error(load_config_directory_loader_errc::invalid_bdd_dynamic_relocations);
			break;
		}
	}

	if (remaining_size)
		bdd.add_error(load_config_directory_loader_errc::invalid_bdd_info_size);
}

void load_func_overrides(const image::image& instance,
	const loader_options& options,
	rva_type current_rva, rva_type block_end_rva,
	rva_type last_override_rva, rva_type bdd_descriptors_rva,
	function_override_dynamic_relocation_details& fixup)
{
	auto& funcs = fixup.get_relocations();
	while (last_override_rva - current_rva >=
		function_override_dynamic_relocation_item<>::descriptor_type::packed_size)
	{
		auto& func = funcs.emplace_back();
		auto& descriptor = func.get_descriptor();

		try
		{
			current_rva += static_cast<rva_type>(
				struct_from_rva(instance, current_rva, descriptor,
					options.include_headers, options.allow_virtual_data).packed_size);
		}
		catch (const std::system_error&)
		{
			func.add_error(
				load_config_directory_loader_errc::invalid_func_override_dynamic_relocation);
			return;
		}

		auto rva_size = descriptor->rva_size;
		if (rva_size % sizeof(rva_type))
		{
			func.add_error(load_config_directory_loader_errc::invalid_func_override_rvas_size);
			return;
		}

		auto last_rvas_rva = current_rva;
		if (!utilities::math::add_if_safe(last_rvas_rva, rva_size)
			|| last_rvas_rva > last_override_rva)
		{
			fixup.add_error(load_config_directory_loader_errc::invalid_func_override_rvas_size);
			return;
		}

		rva_size /= sizeof(rva_type);

		auto& rvas = func.get_rvas();
		rvas.reserve(rva_size);
		while (rva_size--)
		{
			try
			{
				current_rva += static_cast<rva_type>(
					struct_from_rva(instance, current_rva, rvas.emplace_back(),
						options.include_headers, options.allow_virtual_data).packed_size);
			}
			catch (const std::system_error&)
			{
				rvas.pop_back();
				fixup.add_error(load_config_directory_loader_errc::invalid_func_override_rvas);
				return;
			}
		}

		auto base_relocs_size = descriptor->base_reloc_size;
		auto last_base_relocs_rva = current_rva;
		if (!utilities::math::add_if_safe(last_base_relocs_rva, base_relocs_size)
			|| last_base_relocs_rva > last_override_rva)
		{
			fixup.add_error(load_config_directory_loader_errc::invalid_base_relocation_size);
			return;
		}

		load_relocations_no_extra_data(instance, options, current_rva,
			last_base_relocs_rva, func.get_relocations());

		load_bdd_info(instance, options, descriptor->bdd_offset,
			bdd_descriptors_rva, block_end_rva, func.get_bdd_info());

		current_rva = last_base_relocs_rva;
	}

	if (last_override_rva != current_rva)
		fixup.add_error(load_config_directory_loader_errc::invalid_func_override_size);
}

void load_function_override_relocations(const image::image& instance,
	const loader_options& options,
	rva_type current_rva, rva_type last_base_reloc_rva,
	function_override_dynamic_relocation_list_type& list)
{
	while (load_base_relocation(
		instance, options, current_rva, last_base_reloc_rva, list))
	{
		auto& fixups = list.back();
		//size_of_block is validated by load_base_relocation() call
		auto block_end_rva = current_rva + fixups.get_base_relocation()->size_of_block;
		current_rva += 
			std::remove_cvref_t<decltype(fixups.get_base_relocation())>::packed_size;

		if (block_end_rva - current_rva
			< function_override_dynamic_relocation::relocation_header_type::packed_size)
		{
			fixups.add_error(load_config_directory_loader_errc::invalid_func_override_fixup);
			break;
		}

		try
		{
			current_rva += static_cast<rva_type>(
				struct_from_rva(instance, current_rva, fixups.get_header(),
					options.include_headers, options.allow_virtual_data).packed_size);
		}
		catch (const std::system_error&)
		{
			fixups.add_error(load_config_directory_loader_errc::invalid_func_override_fixup);
			break;
		}

		auto last_override_rva = current_rva;
		if (!utilities::math::add_if_safe(last_override_rva,
			fixups.get_header()->func_override_size) || last_override_rva > block_end_rva)
		{
			fixups.add_error(load_config_directory_loader_errc::invalid_func_override_size);
			return;
		}

		rva_type bdd_descriptors_rva = current_rva;
		if (!utilities::math::add_if_safe(bdd_descriptors_rva,
			fixups.get_header()->func_override_size)
			|| bdd_descriptors_rva > block_end_rva)
		{
			fixups.add_error(load_config_directory_loader_errc::invalid_bdd_info_size);
		}

		load_func_overrides(instance, options, current_rva, block_end_rva,
			last_override_rva, bdd_descriptors_rva, fixups);

		//No need to verify current_rva/block_end_rva,
		//as all remaining data is BDD info region
		current_rva = block_end_rva;
	}
}

void load_dynamic_relocation_list(const image::image& instance, const loader_options& options,
	rva_type current_rva, dynamic_relocation_table_details<std::uint64_t>& table,
	dynamic_relocation_table_details<std::uint64_t>::relocation_v1_list_type& list)
{
	auto last_table_rva = current_rva;
	if (!utilities::math::add_if_safe(last_table_rva, table.get_table()->size))
	{
		table.add_error(load_config_directory_loader_errc::invalid_dynamic_relocation_table_v1_size);
		return;
	}

	while (load_dynamic_relocation_struct(
		instance, options, last_table_rva, current_rva, list))
	{
		auto& reloc = list.back();
		const auto& reloc_descriptor = reloc.get_dynamic_relocation().get();

		auto last_base_reloc_rva = current_rva;
		if (!utilities::math::add_if_safe(last_base_reloc_rva, reloc_descriptor.base_reloc_size)
			|| last_base_reloc_rva > last_table_rva)
		{
			reloc.add_error(
				load_config_directory_loader_errc::invalid_dynamic_relocation_header_size);
			break;
		}

		switch (reloc_descriptor.symbol)
		{
		case detail::load_config::dynamic_relocation_symbol::guard_import_control_transfer:
			load_relocations_no_extra_data(instance, options, current_rva, last_base_reloc_rva,
				reloc.get_fixup_lists().emplace<import_control_transfer_dynamic_relocation_list_type>());
			break;

		case detail::load_config::dynamic_relocation_symbol::guard_indir_control_transfer:
			load_relocations_no_extra_data(instance, options, current_rva, last_base_reloc_rva,
				reloc.get_fixup_lists().emplace<indir_control_transfer_dynamic_relocation_list_type>());
			break;

		case detail::load_config::dynamic_relocation_symbol::guard_switchtable_branch:
			load_relocations_no_extra_data(instance, options, current_rva, last_base_reloc_rva,
				reloc.get_fixup_lists().emplace<switchtable_branch_dynamic_relocation_list_type>());
			break;

		case detail::load_config::dynamic_relocation_symbol::guard_arm64x:
			load_arm64x_relocations(instance, options, current_rva, last_base_reloc_rva,
				reloc.get_fixup_lists().emplace<arm64x_dynamic_relocation_list_type>());
			break;

		case detail::load_config::dynamic_relocation_symbol::function_override:
			load_function_override_relocations(instance, options, current_rva, last_base_reloc_rva,
				reloc.get_fixup_lists().emplace<function_override_dynamic_relocation_list_type>());
			break;

		default:
			reloc.add_error(load_config_directory_loader_errc::unknown_dynamic_relocation_symbol);
			break;
		}

		current_rva = last_base_reloc_rva;
	}
}

template<typename Header>
bool load_relocations_header_base(const image::image& instance,
	const loader_options& options,
	rva_type& current_rva, rva_type last_header_rva, Header& header)
{
	if (last_header_rva - current_rva < Header::header_type::packed_size)
		return false;

	try
	{
		auto packed_size = static_cast<rva_type>(
			struct_from_rva(instance, current_rva, header.get_header(),
				options.include_headers, options.allow_virtual_data).packed_size);
		if (!utilities::math::add_if_safe(current_rva, packed_size))
			return false;
	}
	catch (const std::system_error&)
	{
		return false;
	}
	return true;
}

bool load_relocations_header(const image::image& instance, const loader_options& options,
	rva_type current_rva, rva_type last_header_rva,
	prologue_dynamic_relocation_header& header)
{
	if (!load_relocations_header_base(instance,
		options, current_rva, last_header_rva, header))
	{
		return false;
	}

	auto prologue_size = header.get_header()->prologue_byte_count;
	if (last_header_rva - current_rva < prologue_size)
		return false;

	try
	{
		byte_vector_from_rva(instance, current_rva, prologue_size, header.get_data(),
			options.include_headers, options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		return false;
	}

	return true;
}

bool load_relocations_header(const image::image& instance, const loader_options& options,
	rva_type current_rva, rva_type last_header_rva,
	epilogue_dynamic_relocation_header_details& header)
{
	if (!load_relocations_header_base(
		instance, options, current_rva, last_header_rva, header))
	{
		return false;
	}

	if (header.get_header()->branch_descriptor_element_size
		< epilogue_branch_descriptor::descriptor_type::packed_size)
	{
		header.add_error(load_config_directory_loader_errc
			::invalid_dynamic_relocation_epilogue_branch_descriptor_size);
		return false;
	}

	auto branch_descriptor_count = header.get_header()->branch_descriptor_count;
	utilities::safe_uint min_required_size = static_cast<std::uint32_t>(
		branch_descriptor_count) * header.get_header()->branch_descriptor_element_size;

	auto bit_width = std::bit_width(branch_descriptor_count);
	auto bits_for_bit_map = bit_width * static_cast<std::uint64_t>(
		header.get_header()->epilogue_count);
	std::uint64_t bytes_for_bit_map = (bits_for_bit_map + CHAR_BIT - 1u) / CHAR_BIT;

	try
	{
		min_required_size += bytes_for_bit_map;
	}
	catch (const std::system_error&)
	{
		header.add_error(
			load_config_directory_loader_errc::invalid_dynamic_relocation_epilogue_size);
		return false;
	}

	auto size = last_header_rva - current_rva;
	if (min_required_size > size)
	{
		header.add_error(
			load_config_directory_loader_errc::invalid_dynamic_relocation_epilogue_size);
		return false;
	}
	
	auto branch_descriptor_data_rva = static_cast<rva_type>(current_rva
		+ epilogue_branch_descriptor::descriptor_type::packed_size * branch_descriptor_count);
	auto& branch_descriptors = header.get_branch_descriptors();
	auto branch_descriptor_data_size = static_cast<rva_type>(
		header.get_header()->branch_descriptor_element_size
			- epilogue_branch_descriptor::descriptor_type::packed_size);
	for (std::uint16_t i = 0; i != branch_descriptor_count; ++i)
	{
		auto& branch_descriptor = branch_descriptors.emplace_back();
		try
		{
			struct_from_rva(instance, current_rva, branch_descriptor.get_descriptor(),
				options.include_headers, options.allow_virtual_data);
			current_rva += epilogue_branch_descriptor::descriptor_type::packed_size;

			if (branch_descriptor_data_size)
			{
				byte_vector_from_rva(instance, branch_descriptor_data_rva,
					branch_descriptor_data_size, branch_descriptor.get_value(),
					options.include_headers, options.allow_virtual_data);
				branch_descriptor_data_rva += branch_descriptor_data_size;
			}
		}
		catch (const std::system_error&)
		{
			header.add_error(load_config_directory_loader_errc
				::invalid_dynamic_relocation_epilogue_branch_descriptors);
			return false;
		}
	}

	auto& bit_map = header.get_branch_descriptor_bit_map();
	bit_map.set_bit_width(bit_width);
	try
	{
		if (bit_width)
		{
			byte_vector_from_rva(instance, branch_descriptor_data_rva,
				static_cast<std::uint32_t>(bytes_for_bit_map), bit_map.get_data(),
				options.include_headers, options.allow_virtual_data);
		}
	}
	catch (const std::system_error&)
	{
		header.add_error(load_config_directory_loader_errc
			::invalid_dynamic_relocation_epilogue_branch_descriptor_bit_map);
		return false;
	}

	return true;
}

void load_dynamic_relocation_list(const image::image& instance, const loader_options& options,
	rva_type current_rva, dynamic_relocation_table_details<std::uint64_t>& table,
	dynamic_relocation_table_details<std::uint64_t>::relocation_v2_list_type& list)
{
	auto last_table_rva = current_rva;
	if (!utilities::math::add_if_safe(last_table_rva, table.get_table()->size))
	{
		table.add_error(
			load_config_directory_loader_errc::invalid_dynamic_relocation_table_v2_size);
		return;
	}

	while (load_dynamic_relocation_struct(
		instance, options, last_table_rva, current_rva, list))
	{
		auto& reloc = list.back();
		const auto& reloc_descriptor = reloc.get_dynamic_relocation();
		if (reloc_descriptor->header_size < reloc_descriptor.packed_size)
		{
			reloc.add_error(
				load_config_directory_loader_errc::invalid_dynamic_relocation_header_size);
			break;
		}

		auto last_header_rva = current_rva - static_cast<rva_type>(reloc_descriptor.packed_size);
		if (!utilities::math::add_if_safe(last_header_rva, reloc_descriptor->header_size)
			|| last_header_rva > last_table_rva)
		{
			reloc.add_error(
				load_config_directory_loader_errc::invalid_dynamic_relocation_header_size);
			break;
		}

		auto last_base_reloc_rva = last_header_rva;
		if (!utilities::math::add_if_safe(last_base_reloc_rva, reloc_descriptor->fixup_info_size)
			|| last_base_reloc_rva > last_table_rva)
		{
			reloc.add_error(
				load_config_directory_loader_errc::invalid_dynamic_relocation_fixup_info_size);
			break;
		}

		switch (reloc_descriptor->symbol)
		{
		case detail::load_config::dynamic_relocation_symbol::guard_rf_prologue:
			if (!load_relocations_header(instance, options, current_rva, last_header_rva,
				reloc.get_header().emplace<prologue_dynamic_relocation_header>()))
			{
				reloc.add_error(
					load_config_directory_loader_errc::invalid_dynamic_relocation_prologue);
				current_rva = last_base_reloc_rva;
				continue;
			}
			break;

		case detail::load_config::dynamic_relocation_symbol::guard_rf_epilogue:
			if (!load_relocations_header(instance, options, current_rva, last_header_rva,
				reloc.get_header().emplace<epilogue_dynamic_relocation_header_details>()))
			{
				reloc.add_error(
					load_config_directory_loader_errc::invalid_dynamic_relocation_epilogue);
				current_rva = last_base_reloc_rva;
				continue;
			}
			break;

		default:
			reloc.add_error(
				load_config_directory_loader_errc::unknown_dynamic_relocation_symbol);
			current_rva = last_base_reloc_rva;
			continue;
		}

		current_rva = last_header_rva;

		load_relocations_no_extra_data(instance, options,
			current_rva, last_base_reloc_rva, reloc.get_fixup_lists());

		current_rva = last_base_reloc_rva;
	}
}

void load_dynamic_relocation_table(const image::image& instance,
	const loader_options& options,
	load_config_directory_details::underlying_type64& directory)
{
	if (!options.load_dynamic_relocation_table)
		return;

	auto table_offset = directory.get_descriptor()
		->rf_guard.dynamic_value_reloc_table_offset;
	auto table_section_index = directory.get_descriptor()
		->rf_guard.dynamic_value_reloc_table_section;
	if (!table_offset || !table_section_index)
		return;

	--table_section_index;
	const auto& section_headers = instance.get_section_table().get_section_headers();
	if (table_section_index >= section_headers.size())
	{
		directory.add_error(load_config_directory_loader_errc
			::invalid_dynamic_relocation_table_section_index);
		return;
	}

	const auto& section = section_headers[table_section_index];
	if (table_offset >= section.get_virtual_size(
		instance.get_optional_header().get_raw_section_alignment()))
	{
		directory.add_error(load_config_directory_loader_errc
			::invalid_dynamic_relocation_table_section_offset);
		return;
	}

	//Does not overflow, section RVA and size are verified
	utilities::safe_uint<rva_type> current_rva = section.get_rva();
	auto& table = directory.get_dynamic_relocation_table().emplace();
	using table_type = std::remove_cvref_t<decltype(table)>;

	try
	{
		current_rva += table_offset;
		current_rva += struct_from_rva(instance, current_rva.value(), table.get_table(),
			options.include_headers, options.allow_virtual_data).packed_size;
	}
	catch (const std::system_error&)
	{
		directory.get_dynamic_relocation_table().reset();
		directory.add_error(load_config_directory_loader_errc
			::invalid_dynamic_relocation_table);
	}
	
	auto version = table.get_table()->version;
	if (version == 1u)
	{
		load_dynamic_relocation_list(instance, options, current_rva.value(), table,
			table.get_relocations().emplace<table_type::relocation_v1_list_type>());
	}
	else if (version == 2u)
	{
		load_dynamic_relocation_list(instance, options, current_rva.value(), table,
			table.get_relocations().emplace<table_type::relocation_v2_list_type>());
	}
	else
	{
		table.add_error(load_config_directory_loader_errc
			::unknown_dynamic_relocation_table_version);
	}
}

template<typename Directory>
void load_enclave_config(const image::image& instance, const loader_options& options,
	Directory& directory)
{
	if (!options.load_enclave_config)
		return;

	utilities::safe_uint enclave_config_va
		= directory.get_descriptor()->enclave.enclave_configuration_pointer;
	if (!enclave_config_va)
		return;

	auto& config = directory.get_enclave_config().emplace();
	auto& config_descriptor = config.get_descriptor();

	try
	{
		enclave_config_va += struct_from_va(instance, enclave_config_va.value(),
			config_descriptor, options.include_headers, options.allow_virtual_data)
			.packed_size;
	}
	catch (const std::system_error&)
	{
		directory.get_enclave_config().reset();
		directory.add_error(load_config_directory_loader_errc::invalid_enclave_config);
		return;
	}

	if (config_descriptor->size > config_descriptor.packed_size)
	{
		try
		{
			byte_vector_from_va(instance, enclave_config_va.value(),
				config_descriptor->size - config_descriptor.packed_size,
				config.get_extra_data(), options.include_headers, options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			config.add_error(
				load_config_directory_loader_errc::invalid_enclave_config_extra_data);
		}
	}

	auto& imports = config.get_imports();
	utilities::safe_uint current_rva = config_descriptor->import_list;
	using import_descriptor_type = typename Directory::enclave_config_type
		::value_type::enclave_import_list_type::value_type::descriptor_type;
	std::uint32_t extra_import_size
		= config_descriptor->import_entry_size > import_descriptor_type::packed_size
		? config_descriptor->import_entry_size - import_descriptor_type::packed_size
		: 0u;

	auto number_of_imports = config_descriptor->number_of_imports;
	if (number_of_imports > options.max_enclave_number_of_imports)
	{
		number_of_imports = options.max_enclave_number_of_imports;
		config.add_error(
			load_config_directory_loader_errc::invalid_enclave_import_array);
	}

	imports.reserve(number_of_imports);
	for (std::uint32_t i = 0; i != number_of_imports; ++i)
	{
		auto& import = imports.emplace_back();
		auto& import_descriptor = import.get_descriptor();

		try
		{
			current_rva += struct_from_rva(instance, current_rva.value(), import_descriptor,
				options.include_headers, options.allow_virtual_data).packed_size;
		}
		catch (const std::system_error&)
		{
			imports.pop_back();
			config.add_error(
				load_config_directory_loader_errc::invalid_enclave_import_array);
			break;
		}

		if (extra_import_size)
		{
			try
			{
				byte_vector_from_rva(instance, current_rva.value(), extra_import_size,
					import.get_extra_data(), options.include_headers, options.allow_virtual_data);
			}
			catch (const std::system_error&)
			{
				import.add_error(
					load_config_directory_loader_errc::invalid_enclave_import_extra_data);
			}

			try
			{
				current_rva += extra_import_size;
			}
			catch (const std::system_error&)
			{
				config.add_error(
					load_config_directory_loader_errc::invalid_enclave_import_array);
				break;
			}
		}

		if (import.get_match() == enclave_import_match::none)
			continue;

		try
		{
			string_from_rva(instance, import_descriptor->import_name, import.get_name(),
				options.include_headers, options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			import.add_error(load_config_directory_loader_errc::invalid_enclave_import_name);
		}
	}
}

template<typename Table>
void load_volatile_metadata_table(const image::image& instance, 
	const loader_options& options,
	rva_type rva, std::uint32_t size, std::uint32_t max_entry_count,
	load_config_directory_loader_errc unaligned_error,
	load_config_directory_loader_errc invalid_count_error,
	load_config_directory_loader_errc invalid_table_error,
	volatile_metadata_details& config,
	Table& table)
{
	if (!rva || !size)
		return;

	static constexpr auto entry_size = Table::value_type::packed_size;
	if (size % entry_size)
		config.add_error(unaligned_error);

	try
	{
		utilities::safe_uint table_rva = rva;
		std::uint32_t count = size / entry_size;
		if (count > max_entry_count)
		{
			count = max_entry_count;
			config.add_error(invalid_count_error);
		}
		table.reserve(count);
		for (std::uint32_t i = 0u; i != count; ++i)
		{
			table_rva += struct_from_rva(instance, table_rva.value(),
				table.emplace_back(),
				options.include_headers, options.allow_virtual_data).packed_size;
		}
	}
	catch (const std::system_error&)
	{
		table.pop_back();
		config.add_error(invalid_table_error);
	}
}

template<typename Directory>
void load_volatile_metadata(const image::image& instance, const loader_options& options,
	Directory& directory)
{
	if (!options.load_volatile_metadata)
		return;

	auto volatile_metadata_va = directory.get_descriptor()
		->volatile_metadata.volatile_metadata_pointer;
	if (!volatile_metadata_va)
		return;

	auto& config = directory.get_volatile_metadata().emplace();
	auto& config_descriptor = config.get_descriptor();

	try
	{
		struct_from_va(instance, volatile_metadata_va, config_descriptor,
			options.include_headers, options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		directory.get_volatile_metadata().reset();
		directory.add_error(
			load_config_directory_loader_errc::invalid_volatile_metadata);
		return;
	}

	load_volatile_metadata_table(instance, options,
		config_descriptor->volatile_access_table,
		config_descriptor->volatile_access_table_size,
		options.max_volatile_metadata_access_entries,
		load_config_directory_loader_errc::unaligned_volatile_metadata_access_rva_table_size,
		load_config_directory_loader_errc::invalid_volatile_metadata_access_rva_table_entry_count,
		load_config_directory_loader_errc::invalid_volatile_metadata_access_rva_table,
		config, config.get_access_rva_table());

	load_volatile_metadata_table(instance, options,
		config_descriptor->volatile_info_range_table,
		config_descriptor->volatile_info_range_table_size,
		options.max_volatile_metadata_info_range_entries,
		load_config_directory_loader_errc::unaligned_volatile_metadata_range_table_size,
		load_config_directory_loader_errc::invalid_volatile_metadata_range_table_entry_count,
		load_config_directory_loader_errc::invalid_volatile_metadata_range_table,
		config, config.get_range_table());
}

template<typename Directory>
void load_ehcont_targets(const image::image& instance, const loader_options& options,
	Directory& directory)
{
	if (!options.load_ehcont_targets)
		return;

	utilities::safe_uint ehcont_targets_va = directory.get_descriptor()
		->guard_exception_handling.guard_eh_continuation_table;
	if (!ehcont_targets_va)
		return;

	auto& targets = directory.get_eh_continuation_targets().emplace();
	auto count = directory.get_descriptor()
		->guard_exception_handling.guard_eh_continuation_count;
	if (count > options.max_ehcont_targets)
	{
		count = static_cast<decltype(count)>(options.max_ehcont_targets);
		directory.add_error(load_config_directory_loader_errc::invalid_ehcont_targets_count);
	}

	rva_type prev{};
	bool is_sorted = true;
	targets.reserve(static_cast<std::size_t>(count));
	while (count--)
	{
		try
		{
			ehcont_targets_va += struct_from_va(instance, ehcont_targets_va.value(),
				targets.emplace_back(), options.include_headers,
				options.allow_virtual_data).packed_size;
		}
		catch (const std::system_error&)
		{
			targets.pop_back();
			directory.add_error(load_config_directory_loader_errc::invalid_ehcont_targets);
			break;
		}

		try
		{
			[[maybe_unused]] auto first_byte = struct_from_rva<std::uint8_t>(
				instance, targets.back().get(), options.include_headers,
				true);
		}
		catch (const std::system_error&)
		{
			directory.add_error(load_config_directory_loader_errc::invalid_ehcont_target_rvas);
		}

		if (prev > targets.back().get())
			is_sorted = false;
	}

	if (!is_sorted)
		directory.add_error(load_config_directory_loader_errc::unsorted_ehcont_targets);
}

template<typename Directory>
bool load_size_and_descriptor(const image::image& instance,
	const loader_options& options, Directory& directory)
{
	auto load_config_dir_info_rva = instance.get_data_directories().get_directory(
		core::data_directories::directory_type::config)->virtual_address;

	try
	{
		struct_from_rva(instance, load_config_dir_info_rva, directory.get_size(),
			options.include_headers, options.allow_virtual_data);

		rva_type struct_rva = load_config_dir_info_rva
			+ Directory::size_type::packed_size;
		auto struct_buf = section_data_from_rva(instance, struct_rva,
			options.include_headers, options.allow_virtual_data);
		buffers::input_buffer_stateful_wrapper_ref wrapper(*struct_buf);
		directory.get_descriptor().deserialize_until(wrapper,
			directory.get_descriptor_size(), options.allow_virtual_data);
	}
	catch (const std::system_error&)
	{
		directory.add_error(
			load_config_directory_loader_errc::invalid_load_config_directory);
		return false;
	}
	return true;
}

template<typename Directory>
void validate_guard_memcpy_function_pointer(const image::image& instance,
	const loader_options& options, Directory& directory)
{
	auto va = directory.get_descriptor()
		->memcpy_function_pointer.guard_memcpy_function_pointer;
	validate_va(instance, va, options.include_headers, options.allow_virtual_data,
		directory, load_config_directory_loader_errc::invalid_guard_memcpy_function_pointer_va);
}

template<typename Directory>
void validate_cast_guard_os_determined_failure_mode(const image::image& instance,
	const loader_options& options, Directory& directory)
{
	auto va = directory.get_descriptor()
		->mode.cast_guard_os_determined_failure_mode;
	validate_va(instance, va, options.include_headers, true, directory,
		load_config_directory_loader_errc::invalid_cast_guard_os_determined_failure_mode_va);
}

template<typename Directory>
void validate_security_cookie(const image::image& instance,
	const loader_options& options, Directory& directory)
{
	auto va = directory.get_descriptor()->base.security_cookie;
	validate_va(instance, va, options.include_headers, true, directory,
		load_config_directory_loader_errc::invalid_security_cookie_va);
}

template<typename Directory>
void validate_xf_guard_vas(const image::image& instance,
	const loader_options& options, Directory& directory)
{
	const auto& xfg = directory.get_descriptor()->extended_flow_guard;
	validate_va(instance, xfg.guard_xfg_check_function_pointer,
		options.include_headers, true, directory,
		load_config_directory_loader_errc::invalid_guard_xfg_check_function_pointer_va);
	validate_va(instance, xfg.guard_xfg_dispatch_function_pointer,
		options.include_headers, true, directory,
		load_config_directory_loader_errc::invalid_guard_xfg_dispatch_function_pointer_va);
	validate_va(instance, xfg.guard_xfg_table_dispatch_function_pointer,
		options.include_headers, true, directory,
		load_config_directory_loader_errc::invalid_guard_xfg_table_dispatch_function_pointer_va);
}

template<typename Directory>
void load_impl(const image::image& instance,
	const loader_options& options, Directory& directory)
{
	if (!load_size_and_descriptor(instance, options, directory))
		return;

	using enum load_config::version;
	switch (directory.get_version())
	{
	case memcpy_guard:
		validate_guard_memcpy_function_pointer(instance, options, directory);
		[[fallthrough]];
	case cast_guard:
		validate_cast_guard_os_determined_failure_mode(
			instance, options, directory);
		[[fallthrough]];
	case xf_guard:
		validate_xf_guard_vas(instance, options, directory);
		[[fallthrough]];
	case eh_guard:
		load_ehcont_targets(instance, options, directory);
		[[fallthrough]];
	case volatile_metadata:
		load_volatile_metadata(instance, options, directory);
		[[fallthrough]];
	case enclave:
		load_enclave_config(instance, options, directory);
		[[fallthrough]];
	case rf_guard_ex: [[fallthrough]];
	case rf_guard:
		load_dynamic_relocation_table(instance, options, directory);
		[[fallthrough]];
	case hybrid_pe:
		load_chpe_metadata(instance, options, directory);
		[[fallthrough]];
	case cf_guard_ex:
		load_cf_guard_export_suppression_table(instance, options, directory);
		load_cf_guard_longjump_table(instance, options, directory);
		[[fallthrough]];
	case code_integrity: [[fallthrough]];
	case cf_guard:
		load_cf_guard(instance, options, directory);
		[[fallthrough]];
	case seh:
		load_safeseh_handler_table(instance, options, directory);
		[[fallthrough]];
	case base:
		load_lock_prefix_table(instance, options, directory);
		validate_security_cookie(instance, options, directory);
		break;
	default:
		assert(false);
		break;
	}
}

} //namespace

namespace pe_bliss::load_config
{

std::error_code make_error_code(load_config_directory_loader_errc e) noexcept
{
	return { static_cast<int>(e), load_config_directory_loader_error_category_instance };
}

std::optional<load_config_directory_details> load(const image::image& instance,
	const loader_options& options)
{
	std::optional<load_config_directory_details> result;
	if (!instance.get_data_directories().has_config())
		return result;

	auto& directory = result.emplace(instance.is_64bit());
	std::visit([&instance, &options] (auto& directory) {
		load_impl(instance, options, directory);
	}, directory.get_value());

	return result;
}

} //namespace pe_bliss::load_config
