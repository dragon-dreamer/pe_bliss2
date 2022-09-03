#include "load_config_dumper.h"

#include <array>
#include <exception>
#include <functional>
#include <iomanip>
#include <system_error>
#include <variant>
#include <vector>

#include "formatter.h"

#include "buffers/input_memory_buffer.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/load_config/load_config_directory.h"
#include "pe_bliss2/load_config/load_config_directory_loader.h"

namespace
{

std::string get_features(pe_bliss::load_config::version version)
{
	std::string features;
	using enum pe_bliss::load_config::version;
	switch (version)
	{
	case memcpy_guard: [[fallthrough]];
	case cast_guard: [[fallthrough]];
	case xf_guard:  features += "XF Guard, CastGuard, "; [[fallthrough]];
	case eh_guard: features += "EH Guard, "; [[fallthrough]];
	case volatile_metadata: features += "Vilatile Metadata, "; [[fallthrough]];
	case enclave: features += "Enclave, "; [[fallthrough]];
	case rf_guard_ex: features += "RF Guard extended, "; [[fallthrough]];
	case rf_guard: features += "RF Guard, "; [[fallthrough]];
	case hybrid_pe: features += "Hybrid PE, "; [[fallthrough]];
	case cf_guard_ex: features += "CF Guard extended, "; [[fallthrough]];
	case code_integrity: features += "Code Integrity, "; [[fallthrough]];
	case cf_guard: features += "CF Guard, "; [[fallthrough]];
	case seh: features += "SEH, "; [[fallthrough]];
	case base: features += "Base features, "; [[fallthrough]];
	default:
		break;
	}
	if (!features.empty())
	{
		features.pop_back();
		features.pop_back();
	}
	return features;
}

template<typename Table>
void dump_lock_prefix_table(formatter& fmt, const Table& table)
{
	if (!table)
		return;

	fmt.print_structure_name("Lock prefix table VA list");
	fmt.get_stream() << '\n';
	for (const auto& va : table->get_prefix_va_list())
	{
		fmt.print_offsets_and_value(va, true);
		fmt.get_stream() << '\n';
	}

	fmt.get_stream() << '\n';
}

void dump_global_flags(formatter& fmt,
	pe_bliss::load_config::global_flags::value flags,
	std::size_t left_padding)
{
	using enum pe_bliss::load_config::global_flags::value;
	fmt.print_flags(flags, left_padding, {
		{ disable_dbgprint, "DISABLE_DBGPRINT" },
		{ kernel_stack_trace_db, "KERNEL_STACK_TRACE_DB" },
		{ user_stack_trace_db, "USER_STACK_TRACE_DB" },
		{ debug_initial_command, "DEBUG_INITIAL_COMMAND" },
		{ debug_initial_command_ex, "DEBUG_INITIAL_COMMAND_EX" },
		{ heap_disable_coalescing, "HEAP_DISABLE_COALESCING" },
		{ disable_page_kernel_stacks, "DISABLE_PAGE_KERNEL_STACKS" },
		{ disable_protdlls, "DISABLE_PROTDLLS" },
		{ disable_stack_extension, "DISABLE_STACK_EXTENSION" },
		{ critsec_event_creation, "CRITSEC_EVENT_CREATION" },
		{ application_verifier, "APPLICATION_VERIFIER" },
		{ enable_handle_exceptions, "ENABLE_HANDLE_EXCEPTIONS" },
		{ enable_close_exceptions, "ENABLE_CLOSE_EXCEPTIONS" },
		{ enable_csrdebug, "ENABLE_CSRDEBUG" },
		{ enable_exception_logging, "ENABLE_EXCEPTION_LOGGING" },
		{ heap_enable_free_check, "HEAP_ENABLE_FREE_CHECK" },
		{ heap_validate_parameters, "HEAP_VALIDATE_PARAMETERS" },
		{ heap_enable_tagging, "HEAP_ENABLE_TAGGING" },
		{ heap_enable_tag_by_dll, "HEAP_ENABLE_TAG_BY_DLL" },
		{ heap_enable_tail_check, "HEAP_ENABLE_TAIL_CHECK" },
		{ heap_validate_all, "HEAP_VALIDATE_ALL" },
		{ enable_kdebug_symbol_load, "ENABLE_KDEBUG_SYMBOL_LOAD" },
		{ enable_handle_type_tagging, "ENABLE_HANDLE_TYPE_TAGGING" },
		{ heap_page_allocs, "HEAP_PAGE_ALLOCS" },
		{ pool_enable_tagging, "POOL_ENABLE_TAGGING" },
		{ enable_system_crit_breaks, "ENABLE_SYSTEM_CRIT_BREAKS" },
		{ maintain_object_typelist, "MAINTAIN_OBJECT_TYPELIST" },
		{ monitor_silent_process_exit, "MONITOR_SILENT_PROCESS_EXIT" },
		{ show_ldr_snaps, "SHOW_LDR_SNAPS" },
		{ stop_on_exception, "STOP_ON_EXCEPTION" },
		{ stop_on_hung_gui, "STOP_ON_HUNG_GUI" },
		{ stop_on_unhandled_exception, "STOP_ON_UNHANDLED_EXCEPTION" }
	});
}

void dump_process_heap_flags(formatter& fmt,
	pe_bliss::load_config::process_heap_flags::value flags,
	std::size_t left_padding)
{
	using enum pe_bliss::load_config::process_heap_flags::value;
	fmt.print_flags(flags, left_padding, {
		{ heap_create_enable_execute, "HEAP_CREATE_ENABLE_EXECUTE" },
		{ heap_generate_exceptions, "HEAP_GENERATE_EXCEPTIONS" },
		{ heap_no_serialize, "HEAP_NO_SERIALIZE" }
	});
}

void dump_dependent_load_flags(formatter& fmt,
	pe_bliss::load_config::dependent_load_flags::value flags,
	std::size_t left_padding)
{
	using enum pe_bliss::load_config::dependent_load_flags::value;
	fmt.print_flags(flags, left_padding, {
		{ load_library_search_application_dir, "LOAD_LIBRARY_SEARCH_APPLICATION_DIR" },
		{ load_library_search_default_dirs, "LOAD_LIBRARY_SEARCH_DEFAULT_DIRS" },
		{ load_library_search_dll_load_dir, "LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR" },
		{ load_library_search_system32, "LOAD_LIBRARY_SEARCH_SYSTEM32" },
		{ load_library_search_user_dirs, "LOAD_LIBRARY_SEARCH_USER_DIRS" }
	});
}

void dump_guard_flags(formatter& fmt,
	pe_bliss::load_config::guard_flags::value flags,
	std::uint32_t guard_cf_function_table_stride,
	std::size_t left_padding)
{
	using enum pe_bliss::load_config::guard_flags::value;
	fmt.print_flags(flags, left_padding, {
		{ cf_instrumented, "CF_INSTRUMENTED" },
		{ cfw_instrumented, "CFW_INSTRUMENTED" },
		{ cf_function_table_present, "CF_FUNCTION_TABLE_PRESENT" },
		{ security_cookie_unused, "SECURITY_COOKIE_UNUSED" },
		{ protect_delayload_iat, "PROTECT_DELAYLOAD_IAT" },
		{ delayload_iat_in_its_own_section, "DELAYLOAD_IAT_IN_ITS_OWN_SECTION" },
		{ cf_export_suppression_info_present, "CF_EXPORT_SUPPRESSION_INFO_PRESENT" },
		{ cf_enable_export_suppression, "CF_ENABLE_EXPORT_SUPPRESSION" },
		{ cf_longjump_table_present, "CF_LONGJUMP_TABLE_PRESENT" },
		{ rf_instrumented, "RF_INSTRUMENTED" },
		{ rf_enable, "RF_ENABLE" },
		{ rf_strict, "RF_STRICT" },
		{ retpoline_present, "RETPOLINE_PRESENT" },
		{ eh_continuation_table_present_20h1, "EH_CONTINUATION_TABLE_PRESENT_20H1" },
		{ eh_continuation_table_present, "EH_CONTINUATION_TABLE_PRESENT" },
		{ xfg_enabled, "XFG_ENABLED" },
		{ castguard_present, "CASTGUARD_PRESENT" },
		{ memcpy_present, "MEMCPY_PRESENT" }
	});

	if (flags & cf_function_table_present)
	{
		fmt.get_stream() << '\n';
		fmt.get_stream() << std::setw(left_padding) << std::setfill(' ') << "";

		color_changer changer(fmt.get_stream(), fmt.get_color_provider(),
			fmt.flags_fg_color, fmt.flags_bg_color);
		fmt.get_stream() << "Function table stride: "
			<< std::hex << guard_cf_function_table_stride;
	}
}

template<typename HandlerTable>
void dump_handler_table(const char* name, formatter& fmt, const HandlerTable& table)
{
	if (!table)
		return;

	fmt.print_structure_name(name);
	fmt.get_stream() << '\n';
	for (const auto& rva : table->get_handler_list())
	{
		fmt.print_offsets_and_value(rva, true);
		fmt.get_stream() << '\n';
	}

	fmt.get_stream() << '\n';
}

template<typename GuardFunctionTable>
void dump_cf_guard_function_table(const char* name,
	formatter& fmt, const GuardFunctionTable& table, std::uint32_t stride)
{
	if (!table)
		return;

	fmt.print_structure_name(name);
	fmt.get_stream() << '\n';

	for (const auto& func : *table)
	{
		fmt.print_field_name("RVA");
		fmt.print_offsets_and_value(func.get_rva(), true);
		fmt.get_stream() << '\n';

		fmt.print_bytes("Additional data", func.get_additional_data());

		if constexpr (std::is_same_v<std::remove_cvref_t<decltype(func)>,
			pe_bliss::load_config::guard_function_details>)
		{
			fmt.print_errors(func);

			if (stride)
			{
				using enum pe_bliss::load_config::gfids_flags::value;
				fmt.print_field_name("Flags");
				fmt.get_stream() << ' ';
				fmt.print_value(static_cast<std::underlying_type_t<pe_bliss::load_config::gfids_flags::value>>(
					func.get_flags()), true);
				fmt.print_flags(func.get_flags(), 6u, {
					{ fid_suppressed, "FID_SUPPRESSED" },
					{ export_suppressed, "EXPORT_SUPPRESSED" },
					{ fid_langexcpthandler, "FID_LANGEXCPTHANDLER" },
					{ fid_xfg, "FID_XFG" }
				});
				fmt.get_stream() << '\n';
			}

			if (func.get_type_based_hash())
			{
				fmt.print_field_name("XFG type-based hash");
				fmt.get_stream() << ": ";
				fmt.print_offsets_and_value(*func.get_type_based_hash(), true);
				fmt.get_stream() << '\n';
			}
		}
		fmt.get_stream() << '\n';
	}
	fmt.get_stream() << '\n';
}

void dump_chpe_metadata(formatter&, std::monostate) {}

const char* range_code_type_to_string(pe_bliss::load_config::chpe_x86_range_code_type type) noexcept
{
	using enum pe_bliss::load_config::chpe_x86_range_code_type;
	switch (type)
	{
	case arm64: return "ARM64";
	case x86: return "X86";
	default: return "Unknown";
	}
}

const char* range_code_type_to_string(pe_bliss::load_config::chpe_arm64x_range_code_type type) noexcept
{
	using enum pe_bliss::load_config::chpe_arm64x_range_code_type;
	switch (type)
	{
	case arm64: return "ARM64";
	case arm64ec: return "ARM64EC";
	case x64: return "X64";
	default: return "Unknown";
	}
}

template<typename RangeEntries>
void dump_range_entries(formatter& fmt, const RangeEntries& range_entries)
{
	fmt.print_field_name("Range entries");
	fmt.get_stream() << '\n';
	if (range_entries.empty())
		fmt.get_stream() << "No range entries";

	fmt.get_stream() << '\n';
	for (const auto& entry : range_entries)
	{
		fmt.print_field_name("RVA");
		fmt.get_stream() << ' ';
		fmt.print_value(entry.get_rva(), true);

		fmt.get_stream() << ' ';
		fmt.print_field_name("Code type");
		fmt.get_stream() << ' ';
		fmt.print_value(range_code_type_to_string(entry.get_code_type()), true);
		fmt.get_stream() << '\n';

		fmt.print_structure(nullptr, entry.get_entry(), std::array{
			value_info{"start_offset"},
			value_info{"length"}
		});
	}

	fmt.get_stream() << '\n';
}

void dump_chpe_metadata(formatter& fmt, const pe_bliss::load_config::chpe_x86_metadata_details& metadata)
{
	fmt.print_structure_name("CHPE x86 metadata");
	fmt.get_stream() << '\n';
	fmt.print_errors(metadata);

	fmt.print_field_name("Version");
	fmt.print_offsets_and_value(metadata.get_version(), true);
	fmt.get_stream() << "\n\n";
	
	fmt.print_structure("Metadata descriptor", metadata.get_metadata(), std::array{
		value_info{"cphe_code_address_range_offset"},
		value_info{"cphe_code_address_range_count"},
		value_info{"wow_a64_exception_handler_function_pointer"},
		value_info{"wow_a64_dispatch_call_function_pointer"},
		value_info{"wow_a64_dispatch_indirect_call_function_pointer"},
		value_info{"wow_a64_dispatch_indirect_call_cfg_function_pointer"},
		value_info{"wow_a64_dispatch_ret_function_pointer"},
		value_info{"wow_a64_dispatch_ret_leaf_function_pointer"},
		value_info{"wow_a64_dispatch_jump_function_pointer"},
		value_info{"compiler_iat_pointer"},
		value_info{"wow_a64_rdtsc_function_pointer"}
	}, metadata.get_metadata_size());

	dump_range_entries(fmt, metadata.get_range_entries());
}

void dump_chpe_metadata(formatter& fmt, const pe_bliss::load_config::chpe_arm64x_metadata_details& metadata)
{
	fmt.print_structure_name("CHPE ARM64X metadata");
	fmt.get_stream() << '\n';
	fmt.print_errors(metadata);

	fmt.print_field_name("Version");
	fmt.print_offsets_and_value(metadata.get_version(), true);
	fmt.get_stream() << "\n\n";

	fmt.print_structure("Metadata descriptor", metadata.get_metadata(), std::array{
		value_info{"cphe_code_address_range_offset"},
		value_info{"cphe_code_address_range_count"},
		value_info{"x64_code_ranges_to_entry_points_table"},
		value_info{"arm64x_redirection_metadata_table"},
		value_info{"dispatch_call_function_pointer_no_redirection"},
		value_info{"dispatch_return_function_pointer"},
		value_info{"unknown_rva1"},
		value_info{"dispatch_indirect_call_function_pointer"},
		value_info{"dispatch_indirect_call_function_pointer_with_cfg_check"},
		value_info{"alternative_entry_point"},
		value_info{"auxiliary_import_address_table"},
		value_info{"x64_code_ranges_to_entry_points_table_entry_count"},
		value_info{"arm64x_redirection_metadata_table_entry_count"},
		value_info{"unknown_rva2"},
		value_info{"unknown_rva3"},
		value_info{"extra_rfe_table"},
		value_info{"extra_rfe_table_size"},
		value_info{"dispatch_function_pointer"},
		value_info{"copy_of_auxiliary_import_address_table"}
	});

	dump_range_entries(fmt, metadata.get_range_entries());
}

template<typename T>
bool dump_dynamic_relocation_list_base(formatter& fmt,
	const pe_bliss::load_config::dynamic_relocation_list_details<T>& relocs)
{
	fmt.print_structure("Base relocation descriptor",
		relocs.get_base_relocation(), std::array{
		value_info{"virtual_address"},
		value_info{"size_of_block"}
	});

	fmt.print_errors(relocs);

	if (relocs.get_fixups().empty())
	{
		fmt.get_stream() << "No fixups\n\n";
		return false;
	}

	return true;
}

template<typename Fixup>
void dump_fixup_common(formatter& fmt, const Fixup& fixup, pe_bliss::rva_type va)
{
	fmt.print_field_name("Metadata");
	fmt.get_stream() << ": ";
	fmt.print_offsets(fixup.get_relocation());
	fmt.get_stream() << '=';
	if constexpr (!std::remove_cvref_t<decltype(fixup)>::is_scalar)
		fmt.print_value(fixup.get_relocation()->metadata, true);
	else
		fmt.print_value(fixup.get_relocation().get(), true);

	fmt.get_stream() << ' ';
	fmt.print_field_name("Rel offset");
	fmt.get_stream() << ": ";
	fmt.print_value(fixup.get_page_relative_offset(), true);

	fmt.get_stream() << ' ';
	fmt.print_field_name("RVA");
	fmt.get_stream() << ": ";
	fmt.print_value(va + fixup.get_page_relative_offset(), true);
}

void dump_dynamic_relocation_list(formatter& fmt, const pe_bliss::load_config::dynamic_relocation_list_details<
	pe_bliss::load_config::import_control_transfer_dynamic_relocation>& relocs)
{
	auto rva = relocs.get_base_relocation()->virtual_address;
	for (const auto& fixup : relocs.get_fixups())
	{
		dump_fixup_common(fmt, fixup, rva);

		fmt.get_stream() << ' ';
		fmt.print_field_name("IsIndirectCall");
		fmt.get_stream() << ": ";
		fmt.print_string(fixup.is_indirect_call() ? "YES" : "NO");

		fmt.get_stream() << ' ';
		fmt.print_field_name("IAT Index");
		fmt.get_stream() << ": ";
		fmt.print_value(fixup.get_iat_index(), false);

		fmt.get_stream() << '\n';
	}
	fmt.get_stream() << '\n';
}

void dump_dynamic_relocation_list(formatter& fmt, const pe_bliss::load_config::dynamic_relocation_list_details<
	pe_bliss::load_config::indir_control_transfer_dynamic_relocation>& relocs)
{
	auto rva = relocs.get_base_relocation()->virtual_address;
	for (const auto& fixup : relocs.get_fixups())
	{
		dump_fixup_common(fmt, fixup, rva);

		fmt.get_stream() << ' ';
		fmt.print_field_name("IsIndirectCall");
		fmt.get_stream() << ": ";
		fmt.print_string(fixup.is_indirect_call() ? "YES" : "NO");

		fmt.get_stream() << ' ';
		fmt.print_field_name("REXW Prefix");
		fmt.get_stream() << ": ";
		fmt.print_string(fixup.is_rex_w_prefix() ? "YES" : "NO");

		fmt.get_stream() << ' ';
		fmt.print_field_name("CFG Check");
		fmt.get_stream() << ": ";
		fmt.print_string(fixup.is_cfg_check() ? "YES" : "NO");

		fmt.get_stream() << '\n';
	}

	fmt.get_stream() << '\n';
}

const char* cpu_register_to_string(pe_bliss::load_config::switchtable_branch_dynamic_relocation::cpu_register type) noexcept
{
	using enum pe_bliss::load_config::switchtable_branch_dynamic_relocation::cpu_register;
	switch (type)
	{
	case rax: return "RAX";
	case rcx: return "RCX";
	case rdx: return "RDX";
	case rbx: return "RBX";
	case rsp: return "RSP";
	case rbp: return "RBP";
	case rsi: return "RSI";
	case rdi: return "RDI";
	case r8: return "R8";
	case r9: return "R9";
	case r10: return "R10";
	case r11: return "R11";
	case r12: return "R12";
	case r13: return "R13";
	case r14: return "R14";
	case r15: return "R15";
	default: return "Unknown";
	}
}

void dump_dynamic_relocation_list(formatter& fmt, const pe_bliss::load_config::dynamic_relocation_list_details<
	pe_bliss::load_config::switchtable_branch_dynamic_relocation>& relocs)
{
	auto rva = relocs.get_base_relocation()->virtual_address;
	for (const auto& fixup : relocs.get_fixups())
	{
		dump_fixup_common(fmt, fixup, rva);

		fmt.get_stream() << ' ';
		fmt.print_field_name("CPU register");
		fmt.get_stream() << ": ";
		fmt.print_string(cpu_register_to_string(fixup.get_register()));

		fmt.get_stream() << '\n';
	}

	fmt.get_stream() << '\n';
}


const char* func_override_type_to_string(
	pe_bliss::load_config::function_override_base_relocation::type type) noexcept
{
	using enum pe_bliss::load_config::function_override_base_relocation::type;
	switch (type)
	{
	case x64_rel32: return "X64_REL32";
	case arm64_branch26: return "ARM64_BRANCH26";
	case arm64_thunk: return "ARM64_THUNK";
	default: return "Invalid";
	}
}

void dump_dynamic_relocation_list(formatter& fmt, const pe_bliss::load_config::dynamic_relocation_list_details<
	pe_bliss::load_config::function_override_base_relocation>& relocs)
{
	auto rva = relocs.get_base_relocation()->virtual_address;
	for (const auto& fixup : relocs.get_fixups())
	{
		dump_fixup_common(fmt, fixup, rva);

		fmt.get_stream() << ' ';
		fmt.print_field_name("Type");
		fmt.get_stream() << ": ";
		fmt.print_string(func_override_type_to_string(fixup.get_type()));

		fmt.get_stream() << '\n';
	}

	fmt.get_stream() << '\n';
}

void dump_dynamic_relocation_list(formatter& fmt,
	const pe_bliss::load_config::function_override_dynamic_relocation_details& fixup)
{
	fmt.print_structure("Base relocation descriptor",
		fixup.get_base_relocation(), std::array{
		value_info{"virtual_address"},
		value_info{"size_of_block"}
	});

	fmt.print_structure("Function override header",
		fixup.get_header(), std::array{
		value_info{"func_override_size"}
	});

	fmt.print_errors(fixup);

	for (const auto& reloc : fixup.get_relocations())
	{
		fmt.print_structure("Function override dynamic relocation descriptor",
			reloc.get_descriptor(), std::array{
			value_info{"original_rva"},
			value_info{"bdd_offset"},
			value_info{"rva_size"},
			value_info{"base_reloc_size"}
		});

		fmt.print_errors(fixup);

		if (!reloc.get_rvas().empty())
		{
			fmt.print_structure_name("Function override dynamic relocation RVAs");
			for (const auto& rva_descriptor : reloc.get_rvas())
			{
				fmt.print_offsets_and_value(rva_descriptor, true);
				fmt.get_stream() << '\n';
			}
		}
		
		for (const auto& base_reloc : reloc.get_relocations())
		{
			fmt.print_structure("Function override base relocation descriptor",
				base_reloc.get_base_relocation(), std::array{
				value_info{"virtual_address"},
				value_info{"size_of_block"}
			});

			dump_dynamic_relocation_list(fmt, base_reloc);
		}
		
		fmt.print_structure("BDD info descriptor",
			reloc.get_bdd_info().get_descriptor(), std::array{
			value_info{"version"},
			value_info{"bdd_size"}
		});

		for (const auto& bdd_node : reloc.get_bdd_info().get_relocations())
		{
			fmt.print_structure("Function override base relocation descriptor",
				bdd_node, std::array{
				value_info{"left"},
				value_info{"right"},
				value_info{"value"}
			});
		}
	}

	fmt.get_stream() << '\n' << '\n';
}

void dump_arm64x_fixups(formatter& fmt,
	const pe_bliss::load_config::arm64x_dynamic_relocation_zero_fill& reloc)
{
	fmt.get_stream() << ' ';
	fmt.print_structure_name("Zero fill");

	fmt.get_stream() << ' ';
	fmt.print_field_name("Size");
	fmt.get_stream() << ": ";
	fmt.print_value(reloc.get_size(), false);
}

void dump_arm64x_fixups(formatter& fmt,
	const pe_bliss::load_config::arm64x_dynamic_relocation_copy_data_details& reloc)
{
	fmt.get_stream() << ' ';
	fmt.print_structure_name("Copy data");

	fmt.get_stream() << ' ';
	fmt.print_field_name("Size");
	fmt.get_stream() << ": ";
	fmt.print_value(reloc.get_size(), false);

	fmt.get_stream() << '\n';
	fmt.print_bytes("Data", reloc.get_data());
}

void dump_arm64x_fixups(formatter& fmt,
	const pe_bliss::load_config::arm64x_dynamic_relocation_add_delta_details& reloc)
{
	fmt.get_stream() << ' ';
	fmt.print_structure_name("Add delta");

	fmt.get_stream() << ' ';
	fmt.print_field_name("Value");
	fmt.get_stream() << ": ";
	fmt.print_offsets_and_value(reloc.get_value(), true);

	fmt.get_stream() << ' ';
	fmt.print_field_name("Multiplier");
	fmt.get_stream() << ": ";
	fmt.print_value(reloc.get_multiplier()
		== pe_bliss::load_config::arm64x_dynamic_relocation_add_delta_details::multiplier::multiplier_8
		? 8u : 4u, false);

	fmt.get_stream() << ' ';
	fmt.print_field_name("Sign");
	fmt.get_stream() << ": ";
	fmt.print_value(reloc.get_sign()
		== pe_bliss::load_config::arm64x_dynamic_relocation_add_delta_details::sign::plus
		? "PLUS" : "MINUS", false);

	fmt.get_stream() << ' ';
	fmt.print_field_name("Delta");
	fmt.get_stream() << ": ";
	fmt.print_value(reloc.get_delta(), false);
}

template<typename... T>
void dump_dynamic_relocation_list(formatter& fmt, const pe_bliss::load_config::dynamic_relocation_list_details<
	std::variant<T...>>& relocs)
{
	auto rva = relocs.get_base_relocation()->virtual_address;
	for (const auto& fixup : relocs.get_fixups())
	{
		std::visit([&fmt, rva] (const auto& fixup) {
			dump_fixup_common(fmt, fixup, rva);
			dump_arm64x_fixups(fmt, fixup);
		}, fixup);

		fmt.get_stream() << '\n';
	}

	fmt.get_stream() << '\n';
}

const char* dynamic_relocation_symbol_to_string(pe_bliss::load_config::dynamic_relocation_symbol symbol) noexcept
{
	using enum pe_bliss::load_config::dynamic_relocation_symbol;
	switch (symbol)
	{
	case guard_rf_prologue: return "GUARD_RF_PROLOGUE";
	case guard_rf_epilogue: return "GUARD_RF_EPILOGUE";
	case guard_import_control_transfer: return "GUARD_IMPORT_CONTROL_TRANSFER";
	case guard_indir_control_transfer: return "GUARD_INDIR_CONTROL_TRANSFER";
	case guard_switchtable_branch: return "GUARD_SWITCHTABLE_BRANCH";
	case guard_arm64x: return "GUARD_ARM64X";
	case function_override: return "FUNCTION_OVERRIDE";
	default: return "Unknown";
	}
}

void dump_dynamic_relocation_symbol(formatter& fmt,
	pe_bliss::load_config::dynamic_relocation_symbol symbol)
{
	fmt.get_stream() << " (";
	fmt.print_string(dynamic_relocation_symbol_to_string(symbol));
	fmt.get_stream() << ')';
}

void dump_dynamic_relocation_list(formatter& fmt,
	const typename pe_bliss::load_config::dynamic_relocation_table_v2_details<std::uint64_t>::fixup_list_type& relocs)
{
	auto rva = relocs.get_base_relocation()->virtual_address;
	for (const auto& fixup : relocs.get_fixups())
	{
		fmt.print_field_name("Value");
		fmt.get_stream() << ": ";
		fmt.print_offsets_and_value(fixup.get_relocation(), true);
		fmt.get_stream() << ' ';

		fmt.print_field_name("RVA");
		fmt.get_stream() << ": ";
		fmt.print_value(fixup.get_page_relative_offset() + rva, true);
		fmt.get_stream() << '\n';
	}

	fmt.get_stream() << '\n';
}

template<typename Pointer>
void dump_dynamic_relocation_list(formatter& fmt, const std::vector<
	pe_bliss::load_config::dynamic_relocation_table_v1_details<Pointer>>& list)
{
	fmt.print_structure_name("Dynamic relocation table v1 list");
	if (list.empty())
	{
		fmt.get_stream() << "\nEmpty\n\n";
		return;
	}

	fmt.get_stream() << "\n\n";

	for (const auto& fixup_table : list)
	{
		fmt.print_structure("Table v1 descriptor",
			fixup_table.get_dynamic_relocation(), std::array{
			value_info{"symbol", false, std::bind(dump_dynamic_relocation_symbol,
				std::ref(fmt), fixup_table.get_symbol())},
			value_info{"base_reloc_size"}
		});
		fmt.print_errors(fixup_table);

		std::visit([&fmt] (const auto& relocation_list) {
			for (const auto& fixup_list : relocation_list)
			{
				if constexpr (std::is_same_v<std::remove_cvref_t<decltype(fixup_list)>,
					pe_bliss::load_config::function_override_dynamic_relocation_details>)
				{
					dump_dynamic_relocation_list(fmt, fixup_list);
				}
				else
				{
					if (dump_dynamic_relocation_list_base(fmt, fixup_list))
						dump_dynamic_relocation_list(fmt, fixup_list);
				}
			}
		}, fixup_table.get_fixup_lists());
	}
}

void dump_v2_relocation_header(formatter& fmt,
	const pe_bliss::load_config::prologue_dynamic_relocation_header& header)
{
	fmt.print_structure("Prologue dynamic relocation header", header.get_header(), std::array{
		value_info{"prologue_byte_count"}
	});

	fmt.print_bytes("Prologue data", header.get_data());
	fmt.get_stream() << '\n';
}

constexpr void dump_v2_relocation_header(formatter&, std::monostate) noexcept
{
}

void dump_v2_relocation_header(formatter& fmt,
	const pe_bliss::load_config::epilogue_dynamic_relocation_header_details& header)
{
	fmt.print_structure("Epilogue dynamic relocation header", header.get_header(), std::array{
		value_info{"epilogue_count"},
		value_info{"epilogue_byte_count"},
		value_info{"branch_descriptor_element_size"},
		value_info{"branch_descriptor_count"}
	});

	fmt.print_errors(header);

	fmt.print_structure_name("Branch descriptors");
	if (header.get_branch_descriptors().empty())
	{
		fmt.get_stream() << "\nEmpty\n\n";
		return;
	}

	fmt.get_stream() << "\n\n";
	for (const auto& descriptor : header.get_branch_descriptors())
	{
		fmt.print_field_name("Instr size");
		fmt.get_stream() << ": ";
		fmt.print_value(descriptor.get_instr_size(), false);
		fmt.get_stream() << ' ';

		fmt.print_field_name("Disp offset");
		fmt.get_stream() << ": ";
		fmt.print_value(descriptor.get_disp_offset(), false);
		fmt.get_stream() << ' ';

		fmt.print_field_name("Disp size");
		fmt.get_stream() << ": ";
		fmt.print_value(descriptor.get_disp_size(), false);
		fmt.get_stream() << '\n';

		fmt.print_bytes("Descriptor data", descriptor.get_value());
		fmt.get_stream() << '\n';
	}

	fmt.get_stream() << '\n';

	auto& bit_map = header.get_branch_descriptor_bit_map();
	auto bit_width = bit_map.get_bit_width();
	auto bit_stream = bit_map.to_bit_stream();

	fmt.print_structure_name("Branch descriptors bit map");
	if (!header.get_header()->epilogue_count)
	{
		fmt.get_stream() << "\nEmpty\n\n";
		return;
	}

	fmt.get_stream() << '\n';
	fmt.print_field_name("Bit width");
	fmt.get_stream() << ": ";
	fmt.print_value(bit_width, false);
	fmt.get_stream() << '\n';
	fmt.print_field_name("Branch indices");
	fmt.get_stream() << '\n';

	for (std::size_t i = 0; i != header.get_header()->epilogue_count; ++i)
	{
		try
		{
			fmt.print_value(bit_stream.read(bit_width), true);
			if (!((i + 1) % 8))
				fmt.get_stream() << '\n';
			else
				fmt.get_stream() << ' ';
		}
		catch (const std::system_error&)
		{
			return;
		}
	}
	fmt.get_stream() << '\n';
}

template<typename Pointer>
void dump_dynamic_relocation_list(formatter& fmt, const std::vector<
	pe_bliss::load_config::dynamic_relocation_table_v2_details<Pointer>>& list)
{
	fmt.print_structure_name("Dynamic relocation table v2 list");
	if (list.empty())
	{
		fmt.get_stream() << "\nEmpty\n\n";
		return;
	}

	fmt.get_stream() << "\n\n";
	
	for (const auto& fixup_table : list)
	{
		fmt.print_structure("Table v2 descriptor",
			fixup_table.get_dynamic_relocation(), std::array{
			value_info{"header_size"},
			value_info{"fixup_info_size"},
			value_info{"symbol", false, std::bind(dump_dynamic_relocation_symbol,
				std::ref(fmt), fixup_table.get_symbol())},
			value_info{"symbol_group"},
			value_info{"flags"}
		});
		fmt.print_errors(fixup_table);

		std::visit([&fmt] (const auto& header) {
			dump_v2_relocation_header(fmt, header);
		}, fixup_table.get_header());

		for (const auto& fixup_list : fixup_table.get_fixup_lists())
		{
			if (dump_dynamic_relocation_list_base(fmt, fixup_list))
				dump_dynamic_relocation_list(fmt, fixup_list);
		}
	}
}

constexpr void dump_dynamic_relocation_list(formatter&, std::monostate) {}

template<typename RelocationTable>
void dump_dynamic_relocation_table(formatter& fmt, const RelocationTable& table)
{
	if (!table)
		return;

	fmt.print_structure_name("Dynamic relocation table");
	fmt.get_stream() << "\n\n";

	fmt.print_structure("Table descriptor", table->get_table(), std::array{
		value_info{"version"},
		value_info{"size"}
	});

	fmt.print_errors(*table);

	std::visit([&fmt] (const auto& reloc_list) {
		dump_dynamic_relocation_list(fmt, reloc_list);
	}, table->get_relocations());
}

void dump_policy_flags(formatter& fmt,
	pe_bliss::load_config::enclave_policy_flags::value flags,
	std::size_t left_padding)
{
	using enum pe_bliss::load_config::enclave_policy_flags::value;
	fmt.print_flags(flags, left_padding, {
		{ debuggable, "DEBUGGABLE" }
	});
}

void dump_enclave_flags(formatter& fmt,
	pe_bliss::load_config::enclave_flags::value flags,
	std::size_t left_padding)
{
	using enum pe_bliss::load_config::enclave_flags::value;
	fmt.print_flags(flags, left_padding, {
		{ primary_image, "PRIMARY_IMAGE" }
	});
}

struct dump_enclave_id
{
	template<std::size_t N>
	void operator()(formatter& fmt, const std::uint8_t(&arr)[N], std::size_t left_padding) const
	{
		color_changer changer(fmt.get_stream(), fmt.get_color_provider(),
			fmt.value_fg_color, fmt.value_bg_color);

		for (std::size_t i = 0; i != N; ++i)
		{
			if (!(i % 8) && i)
			{
				fmt.get_stream() << '\n'
					<< std::setw(left_padding) << std::setfill(' ') << "";
			}
			fmt.get_stream() << std::hex << std::setw(2) << std::setfill('0')
				<< static_cast<std::uint32_t>(arr[i]) << ' ';
		}
	}
};

const char* enclave_import_match_type_to_string(pe_bliss::load_config::enclave_import_match match) noexcept
{
	using enum pe_bliss::load_config::enclave_import_match;
	switch (match)
	{
	case none: return "NONE";
	case unique_id: return "UNIQUE_ID";
	case author_id: return "AUTHOR_ID";
	case family_id: return "FAMILY_ID";
	case image_id: return "IMAGE_ID";
	default: return "Unknown";
	}
}

void dump_enclave_import_match_type(formatter& fmt, pe_bliss::load_config::enclave_import_match match)
{
	fmt.get_stream() << " (";
	fmt.print_string(enclave_import_match_type_to_string(match));
	fmt.get_stream() << ')';
}

template<typename Config>
void dump_enclave_config(formatter& fmt, const Config& config)
{
	if (!config)
		return;

	fmt.print_structure_name("Enclave config");
	fmt.get_stream() << "\n\n";

	fmt.print_structure("Enclave config descriptor", config->get_descriptor(), std::array{
		value_info{"size"},
		value_info{"minimum_required_config_size"},
		value_info{"policy_flags", true, std::bind(dump_policy_flags,
			std::ref(fmt), config->get_policy_flags(), std::placeholders::_1)},
		value_info{"number_of_imports"},
		value_info{"import_list"},
		value_info{"import_entry_size"},
		value_info{"family_id", true, std::bind(dump_enclave_id{},
			std::ref(fmt), std::cref(config->get_descriptor()->family_id), std::placeholders::_1), false},
		value_info{"image_id", true, std::bind(dump_enclave_id{},
			std::ref(fmt), std::cref(config->get_descriptor()->image_id), std::placeholders::_1), false},
		value_info{"image_version"},
		value_info{"security_version"},
		value_info{"enclave_size"},
		value_info{"number_of_threads"},
		value_info{"enclave_flags", true, std::bind(dump_enclave_flags,
			std::ref(fmt), config->get_flags(), std::placeholders::_1)}
	});

	if (!config->get_extra_data().value().empty())
		fmt.print_bytes("Extra data", config->get_extra_data());

	fmt.print_errors(*config);
	fmt.get_stream() << '\n';

	fmt.print_structure_name("Enclave imports");
	if (config->get_imports().empty())
	{
		fmt.get_stream() << "\nEmpty\n\n";
		return;
	}

	fmt.get_stream() << '\n';
	for (const auto& import : config->get_imports())
	{
		fmt.print_structure("Enclave import descriptor", import.get_descriptor(), std::array{
			value_info{"match_type", true, std::bind(dump_enclave_import_match_type,
				std::ref(fmt), import.get_match())},
			value_info{"minimum_security_version"},
			value_info{"unique_or_author_id", true, std::bind(dump_enclave_id{},
				std::ref(fmt), std::cref(import.get_descriptor()->unique_or_author_id), std::placeholders::_1), false},
			value_info{"family_id", true, std::bind(dump_enclave_id{},
				std::ref(fmt), std::cref(import.get_descriptor()->family_id), std::placeholders::_1), false},
			value_info{"image_id", true, std::bind(dump_enclave_id{},
				std::ref(fmt), std::cref(import.get_descriptor()->image_id), std::placeholders::_1), false},
			value_info{"import_name", true, std::bind(
				&formatter::print_packed_string<pe_bliss::packed_c_string>, std::ref(fmt),
				std::cref(import.get_name()))},
			value_info{"reserved"}
		});

		if (!import.get_extra_data().value().empty())
			fmt.print_bytes("Extra data", import.get_extra_data());
	}

	fmt.get_stream() << '\n';
}

template<typename Metadata>
void dump_volatile_metadata(formatter& fmt, const Metadata& metadata)
{
	if (!metadata)
		return;

	fmt.print_structure_name("Volatile metadata");
	fmt.get_stream() << "\n\n";

	fmt.print_structure("Volatile metadata descriptor", metadata->get_descriptor(), std::array{
		value_info{"size"},
		value_info{"version"},
		value_info{"volatile_access_table"},
		value_info{"volatile_access_table_size"},
		value_info{"volatile_info_range_table"},
		value_info{"volatile_info_range_table_size"}
	});

	fmt.print_errors(*metadata);
	fmt.get_stream() << '\n';

	fmt.print_structure_name("Volatile metadata access RVA table");
	if (metadata->get_access_rva_table().empty())
	{
		fmt.get_stream() << "\nEmpty\n\n";
	}
	else
	{
		fmt.get_stream() << '\n';
		for (const auto& rva : metadata->get_access_rva_table())
		{
			fmt.print_field_name("RVA");
			fmt.print_offsets_and_value(rva, true);
			fmt.get_stream() << '\n';
		}
	}

	fmt.print_structure_name("Volatile metadata range table");
	if (metadata->get_range_table().empty())
	{
		fmt.get_stream() << "\nEmpty\n\n";
	}
	else
	{
		fmt.get_stream() << '\n';
		for (const auto& entry : metadata->get_range_table())
		{
			fmt.print_structure("Entry descriptor", entry, std::array{
				value_info{"rva"},
				value_info{"size"}
			});
		}
	}
	fmt.get_stream() << '\n';
}

template<typename Targets>
void dump_ehcont_targets(formatter& fmt, const Targets& targets)
{
	if (!targets)
		return;

	fmt.print_structure_name("Exception handling continuation targets RVA table");
	if (targets->empty())
	{
		fmt.get_stream() << "\nEmpty\n\n";
		return;
	}

	fmt.get_stream() << '\n';
	for (const auto& rva : *targets)
	{
		fmt.print_field_name("RVA");
		fmt.print_offsets_and_value(rva, true);
		fmt.get_stream() << '\n';
	}
	fmt.get_stream() << '\n';
}

template<typename LoadConfig>
void dump_load_config_impl(formatter& fmt, const LoadConfig& directory)
{
	fmt.print_field_name("Version");
	fmt.get_stream() << ": ";
	fmt.print_string(pe_bliss::load_config::version_to_min_required_windows_version(directory.get_version()));

	fmt.get_stream() << " (exact match: ";
	fmt.print_string(directory.version_exactly_matches() ? "YES" : "NO");
	fmt.get_stream() << ")\n";

	fmt.print_field_name("Size");
	fmt.get_stream() << ": ";
	fmt.print_offsets_and_value(directory.get_size(), true);
	fmt.get_stream() << '\n';

	fmt.print_field_name("Supported features");
	fmt.get_stream() << ": ";
	fmt.print_string(get_features(directory.get_version()).c_str());
	fmt.get_stream() << "\n\n";

	fmt.print_structure("Load config descriptor", directory.get_descriptor(), std::array{
		value_info{"time_date_stamp"},
		value_info{"major_version"},
		value_info{"minor_version"},
		value_info{"global_flags_clear", true, std::bind(dump_global_flags,
			std::ref(fmt), directory.get_global_flags_clear(), std::placeholders::_1)},
		value_info{"global_flags_set", true, std::bind(dump_global_flags,
			std::ref(fmt), directory.get_global_flags_set(), std::placeholders::_1)},
		value_info{"critical_section_default_timeout"},
		value_info{"de_commit_free_block_threshold"},
		value_info{"de_commit_total_free_threshold"},
		value_info{"lock_prefix_table"},
		value_info{"maximum_allocation_size"},
		value_info{"virtual_memory_threshold"},
		value_info{"process_heap_flags", true, std::bind(dump_process_heap_flags,
			std::ref(fmt), directory.get_process_heap_flags(), std::placeholders::_1)},
		value_info{"process_affinity_mask"},
		value_info{"csd_version"},
		value_info{"dependent_load_flags", true, std::bind(dump_dependent_load_flags,
			std::ref(fmt), directory.get_dependent_load_flags(), std::placeholders::_1)},
		value_info{"edit_list"},
		value_info{"security_cookie"},
		value_info{"se_handler_table"},
		value_info{"se_handler_count"},
		value_info{"guard_cf_check_function_pointer"},
		value_info{"guard_cf_dispatch_function_pointer"},
		value_info{"guard_cf_function_table"},
		value_info{"guard_cf_function_count"},
		value_info{"guard_flags", true, std::bind(dump_guard_flags,
			std::ref(fmt), directory.get_guard_flags(), directory.get_guard_cf_function_table_stride(),
			std::placeholders::_1)},
		value_info{"code_integrity.flags"},
		value_info{"code_integrity.catalog"},
		value_info{"code_integrity.catalog_offset"},
		value_info{"code_integrity.reserved"},
		value_info{"guard_address_taken_iat_entry_table"},
		value_info{"guard_address_taken_iat_entry_count"},
		value_info{"guard_long_jump_target_table"},
		value_info{"guard_long_jump_target_count"},
		value_info{"dynamic_value_reloc_table"},
		value_info{"chpe_metadata_pointer"},
		value_info{"guard_rf_failure_routine"},
		value_info{"guard_rf_failure_routine_function_pointer"},
		value_info{"dynamic_value_reloc_table_offset"},
		value_info{"dynamic_value_reloc_table_section"},
		value_info{"reserved2"},
		value_info{"guard_rf_verify_stack_pointer_function_pointer"},
		value_info{"hot_patch_table_offset"},
		value_info{"reserved3"},
		value_info{"enclave_configuration_pointer"},
		value_info{"volatile_metadata_pointer"},
		value_info{"guard_eh_continuation_table"},
		value_info{"guard_eh_continuation_count"},
		value_info{"guard_xfg_check_function_pointer"},
		value_info{"guard_xfg_dispatch_function_pointer"},
		value_info{"guard_xfg_table_dispatch_function_pointer"},
		value_info{"cast_guard_os_determined_failure_mode"},
		value_info{"guard_memcpy_function_pointer"}
	}, directory.get_descriptor_size());

	fmt.print_errors(directory);

	dump_lock_prefix_table(fmt, directory.get_lock_prefix_table());
	dump_handler_table("SEH handler RVA table", fmt, directory.get_safeseh_handler_table());
	dump_cf_guard_function_table("Guard CF function table",
		fmt, directory.get_guard_cf_function_table(),
		directory.get_guard_cf_function_table_stride());
	dump_cf_guard_function_table("Guard CF address taken IAT entry table",
		fmt, directory.get_guard_address_taken_iat_entry_table(),
		directory.get_guard_cf_function_table_stride());
	dump_cf_guard_function_table("Guard CF long jump target table",
		fmt, directory.get_guard_long_jump_target_table(),
		directory.get_guard_cf_function_table_stride());
	std::visit([&fmt] (const auto& metadata) { dump_chpe_metadata(fmt, metadata); },
		directory.get_chpe_metadata());
	dump_dynamic_relocation_table(fmt, directory.get_dynamic_relocation_table());
	dump_enclave_config(fmt, directory.get_enclave_config());
	dump_volatile_metadata(fmt, directory.get_volatile_metadata());
	dump_ehcont_targets(fmt, directory.get_eh_continuation_targets());
}

} //namespace

void dump_load_config(formatter& fmt, const pe_bliss::image::image& image) try
{
	auto load_config = pe_bliss::load_config::load(image, {});
	if (!load_config)
		return;

	fmt.get_stream() << "===== ";
	fmt.print_structure_name("Load config directory");
	fmt.get_stream() << " =====\n\n";

	std::visit([&fmt] (const auto& load_config) { dump_load_config_impl(fmt, load_config); },
		load_config->get_value());
}
catch (const std::system_error& e)
{
	fmt.print_error("Error loading load config:", e);
}
catch (const std::exception& e)
{
	fmt.print_error("Error loading load config:", e);
}
