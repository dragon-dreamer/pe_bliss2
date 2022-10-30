#include "debug_dumper.h"

#include <array>
#include <functional>
#include <string>
#include <type_traits>
#include <variant>

#include "formatter.h"

#include "pe_bliss2/debug/debug_directory.h"
#include "pe_bliss2/debug/debug_directory_loader.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/packed_c_string.h"

namespace
{

const char* debug_type_to_string(pe_bliss::debug::debug_directory_type type)
{
	using enum pe_bliss::debug::debug_directory_type;
	switch (type)
	{
	case coff: return "COFF";
	case codeview: return "CodeView";
	case fpo: return "FPO";
	case misc: return "Misc";
	case exception: return "Execution";
	case fixup: return "Fixup";
	case omap_to_src: return "OmapToSrc";
	case omap_from_src: return "OmapFromSrc";
	case borland: return "Borland";
	case bbt: return "BBT";
	case clsid: return "CLSID";
	case vc_feature: return "VC Feature";
	case pogo: return "POGO";
	case iltcg: return "ILTCG";
	case mpx: return "MPX";
	case repro: return "Repro";
	case mpdb: return "MPDB";
	case spgo: return "SPGO";
	case pdbhash: return "PDB Hash";
	case ex_dllcharacteristics: return "Ex DLL Characteristics";
	default: return "Unknown";
	}
}

void print_debug_type(formatter& fmt, pe_bliss::debug::debug_directory_type type)
{
	fmt.get_stream() << '(';
	fmt.print_string(debug_type_to_string(type));
	fmt.get_stream() << ')';
}

void print_coff_symbol_name(formatter& fmt,
	const pe_bliss::debug::coff_symbol::name_type& name, std::size_t arr_index)
{
	if (arr_index != 0u)
		return;

	fmt.get_stream() << '(';
	const auto* str = std::get_if<std::string>(&name);
	if (str)
		fmt.print_string(*str);
	else
		fmt.print_packed_string(std::get<pe_bliss::packed_c_string>(name));
	fmt.get_stream() << ')';
}

void print_coff_section_number(formatter& fmt, std::int16_t section_number)
{
	switch (section_number)
	{
	case pe_bliss::debug::coff_symbol::section_number_absolute:
		fmt.print_string("ABSOLUTE");
		return;

	case pe_bliss::debug::coff_symbol::section_number_debug:
		fmt.print_string("DEBUG");
		return;

	case pe_bliss::debug::coff_symbol::section_number_undef:
		fmt.print_string("UNDEF");
		return;
	}
}

const char* coff_symbol_type_to_string(pe_bliss::debug::coff_symbol_type type)
{
	using enum pe_bliss::debug::coff_symbol_type;
	switch (type)
	{
	case sym_null: return "NULL";
	case sym_void: return "VOID";
	case sym_char: return "CHAR";
	case sym_short: return "SHORT";
	case sym_int: return "INT";
	case sym_long: return "LONG";
	case sym_float: return "FLOAT";
	case sym_double: return "DOUBLE";
	case sym_struct: return "STRUCT";
	case sym_union: return "UNION";
	case sym_enum: return "ENUM";
	case sym_member_of_enum: return "MEMBER OF ENUM";
	case sym_uchar: return "UCHAR";
	case sym_ushort: return "USHORT";
	case sym_uint: return "UINT";
	case sym_ulong: return "ULONG";
	case sym_long_double: return "LONG DOUBLE";
	default: return "Unknown";
	}
}

const char* coff_symbol_attr_to_string(pe_bliss::debug::coff_symbol_type_attr attr)
{
	using enum pe_bliss::debug::coff_symbol_type_attr;
	switch (attr)
	{
	case no_derived: return "NO DERIVED";
	case pointer: return "POINTER";
	case function_return_value: return "FUNCTION RETURN VALUE";
	case array: return "ARRAY";
	default: return "Unknown";
	}
}

void print_coff_symbol_type(formatter& fmt, pe_bliss::debug::coff_symbol_type type,
	pe_bliss::debug::coff_symbol_type_attr attr)
{
	fmt.get_stream() << '(';
	fmt.print_string(coff_symbol_type_to_string(type));
	fmt.get_stream() << ", ";
	fmt.print_string(coff_symbol_attr_to_string(attr));
	fmt.get_stream() << ')';
}

const char* coff_storage_class_to_string(pe_bliss::debug::coff_storage_class type)
{
	using enum pe_bliss::debug::coff_storage_class;
	switch (type)
	{
	case null: return "NULL";
	case automatic: return "AUTOMATIC";
	case external: return "EXTERNAL";
	case static_class: return "STATIC CLASS";
	case register_class: return "REGISTER CLASS";
	case external_definition: return "EXTERNAL DEFINITION";
	case label: return "LABEL";
	case undefined_label: return "UNDEFINED LABEL";
	case member_of_structure: return "MEMBER OF STRUCTURE";
	case function_argument: return "FUNCTION ARGUMENT";
	case structure_tag: return "STRUCTURE TAG";
	case member_of_union: return "MEMBER OF UNION";
	case union_tag: return "UNION TAG";
	case type_definition: return "TYPE DEFINITION";
	case undefined_static: return "UNDEFINED STATIC";
	case enumeration_tag: return "ENUMERATION TAG";
	case member_of_enumeration: return "MEMBER OF ENUMERATION";
	case register_param: return "REGISTER PARAM";
	case bit_field: return "BIT FIELD";
	case automatic_argument: return "AUTOMATIC ARGUMENT";
	case dummy_entry: return "DUMMY ENTRY";
	case begin_or_end_of_block: return "BEGIN OR END OF BLOCK";
	case begin_or_end_of_function: return "BEGIN OR END OF FUNCTION";
	case end_of_structure: return "END OF STRUCTURE";
	case file_name: return "FILE NAME";
	case line_number: return "LINE NUMBER";
	case duplicate_tag: return "DUPLICATE TAG";
	case hidden: return "HIDDEN";
	case physical_end_of_function: return "PHYSICAL END OF FUNCTION";
	default: return "Unknown";
	}
}

void print_coff_storage_class(formatter& fmt, pe_bliss::debug::coff_storage_class type)
{
	fmt.get_stream() << '(';
	fmt.print_string(coff_storage_class_to_string(type));
	fmt.get_stream() << ')';
}

void dump_debug_directory(formatter& fmt,
	const pe_bliss::debug::coff_debug_directory_details& dir)
{
	fmt.print_structure("COFF symbol header", dir.get_descriptor(), std::array{
		value_info{"number_of_symbols"},
		value_info{"lva_to_first_symbol"},
		value_info{"number_of_linenumbers"},
		value_info{"lva_to_first_linenumber"},
		value_info{"rva_to_first_byte_of_code"},
		value_info{"rva_to_last_byte_of_code"},
		value_info{"rva_to_first_byte_of_data"},
		value_info{"rva_to_last_byte_of_data"}
	});

	fmt.print_field_name("COFF string table length");
	fmt.print_offsets_and_value(dir.get_string_table_length(), true);
	if (!dir.get_symbols().empty())
		fmt.get_stream() << "\n\n";

	for (const auto& symbol : dir.get_symbols())
	{
		fmt.print_structure("COFF symbol", symbol.get_descriptor(), std::array{
			value_info{"name", true, std::bind(
				print_coff_symbol_name, std::ref(fmt),
				std::cref(symbol.get_name()), std::placeholders::_2)},
			value_info{"value"},
			value_info{"section_number", false, std::bind(
				print_coff_section_number, std::ref(fmt),
				symbol.get_descriptor()->section_number)},
			value_info{"type", true, std::bind(
				print_coff_symbol_type, std::ref(fmt),
				symbol.get_type(), symbol.get_type_attr())},
			value_info{"storage_class", true, std::bind(
				print_coff_storage_class, std::ref(fmt),
				symbol.get_storage_class())},
			value_info{"aux_symbol_number"}
		});
	}
}

void dump_debug_directory(formatter& fmt,
	const pe_bliss::debug::codeview_pdb7_debug_directory_details& dir)
{
	fmt.print_structure("CodeView PDB7 descriptor", dir.get_descriptor(), std::array{
		value_info{"cv_signature"},
		value_info{"signature"},
		value_info{"age"}
	});
	fmt.print_field_name("PDB name");
	fmt.print_packed_string(dir.get_pdb_file_name());
}

void dump_debug_directory(formatter& fmt,
	const pe_bliss::debug::codeview_pdb2_debug_directory_details& dir)
{
	fmt.print_structure("CodeView PDB2 descriptor", dir.get_descriptor(), std::array{
		value_info{"cv_signature"},
		value_info{"offset"},
		value_info{"signature"},
		value_info{"age"}
	});
	fmt.print_field_name("PDB name");
	fmt.print_packed_string(dir.get_pdb_file_name());
}

void dump_debug_directory(formatter& fmt,
	const pe_bliss::debug::codeview_omf_debug_directory_details& dir)
{
	fmt.print_field_name("CodeView directory signature");
	fmt.print_offsets_and_value(dir.get_signature(), true);
}

const char* fpo_frame_type_to_string(pe_bliss::debug::fpo_frame_type type)
{
	using enum pe_bliss::debug::fpo_frame_type;
	switch (type)
	{
	case fpo: return "FPO";
	case trap: return "TRAP";
	case tss: return "TSS";
	case nonfpo: return "NONFPO";
	default: return "Unknown";
	}
}

void dump_debug_directory(formatter& fmt,
	const pe_bliss::debug::fpo_debug_directory_details& dir)
{
	fmt.print_structure_name("FPO entries");
	for (const auto& entry : dir.get_entries())
	{
		fmt.get_stream() << "\n\n";
		fmt.print_structure("FPO entry descriptor", entry.get_descriptor(), std::array{
			value_info{"ul_off_start"},
			value_info{"cb_proc_size"},
			value_info{"cdw_locals"},
			value_info{"cdw_params"},
			value_info{"flags"}
		});
		fmt.print_field_name("Is EBP Allocated");
		fmt.get_stream() << ": ";
		fmt.print_string(entry.is_ebp_allocated() ? "YES" : "NO");
		fmt.get_stream() << '\n';

		fmt.print_field_name("Is SEH in func");
		fmt.get_stream() << ": ";
		fmt.print_string(entry.is_seh_in_func() ? "YES" : "NO");
		fmt.get_stream() << '\n';

		fmt.print_field_name("Prolog byte count");
		fmt.get_stream() << ": ";
		fmt.print_value(entry.get_prolog_byte_count());
		fmt.get_stream() << '\n';

		fmt.print_field_name("Saved regs count");
		fmt.get_stream() << ": ";
		fmt.print_value(entry.get_saved_regs());
		fmt.get_stream() << '\n';

		fmt.print_field_name("Frame type");
		fmt.get_stream() << ": ";
		fmt.print_string(fpo_frame_type_to_string(entry.get_frame_type()));
	}
}

void print_misc_data_type(formatter& fmt, pe_bliss::debug::misc_data_type type)
{
	fmt.get_stream() << '(';
	fmt.print_string(type == pe_bliss::debug::misc_data_type::exename ? "Exe name" : "Unknown");
	fmt.get_stream() << ')';
}

void dump_debug_directory(formatter& fmt,
	const pe_bliss::debug::misc_debug_directory_details& dir)
{
	fmt.print_structure("Misc descriptor", dir.get_descriptor(), std::array{
		value_info{"data_type", true, std::bind(
			print_misc_data_type, std::ref(fmt), dir.get_data_type())},
		value_info{"length"},
		value_info{"unicode"},
		value_info{"reserved"}
	});

	fmt.print_field_name("Data");
	std::visit([&fmt](const auto& data) {
		if constexpr (std::is_same_v<const std::string&, decltype(data)>)
			fmt.print_string(data);
		else
			fmt.print_packed_string(data);
	}, dir.get_data());
}

void dump_debug_directory(formatter& fmt,
	const pe_bliss::debug::omap_to_src_debug_directory_details& dir)
{
	fmt.print_structure_name("OMAP to SRC entries");
	for (const auto& entry : dir.get_mappings())
	{
		fmt.print_structure("OMAP to SRC mapping", entry, std::array{
			value_info{"rva"},
			value_info{"rva_to"}
		});
	}
}

void dump_debug_directory(formatter& fmt,
	const pe_bliss::debug::src_to_omap_debug_directory_details& dir)
{
	fmt.print_structure_name("SRC to OMAP entries");
	for (const auto& entry : dir.get_mappings())
	{
		fmt.print_structure("SRC to OMAP mapping", entry, std::array{
			value_info{"rva"},
			value_info{"rva_to"}
		});
	}
}

void dump_debug_directory(formatter& fmt,
	const pe_bliss::debug::vc_feature_debug_directory_details& dir)
{
	fmt.print_structure("VC Feature descriptor", dir.get_descriptor(), std::array{
		value_info{"pre_vc_plus_plus_11_count"},
		value_info{"c_and_c_plus_plus_count"},
		value_info{"gs_count"},
		value_info{"sdl_count"},
		value_info{"guard_n_count"}
	});
}

const char* pogo_type_to_string(pe_bliss::debug::pogo_type type)
{
	using enum pe_bliss::debug::pogo_type;
	switch (type)
	{
	case ltcg: return "LTCG, link-time code generation";
	case pgu: return "PGU, profile guided update";
	case pgi: return "PGI, profile guided instrumentation";
	case pgo: return "PGO, profile guided optimization";
	default: return "Unknown";
	}
}

void print_pogo_type(formatter& fmt, pe_bliss::debug::pogo_type type)
{
	fmt.get_stream() << '(';
	fmt.print_string(pogo_type_to_string(type));
	fmt.get_stream() << ')';
}

void dump_debug_directory(formatter& fmt,
	const pe_bliss::debug::pogo_debug_directory_details& dir)
{
	fmt.print_structure("POGO header", dir.get_descriptor(), std::array{
		value_info{"signature", true, std::bind(
			print_pogo_type, std::ref(fmt), dir.get_pogo_type())},
	});

	for (const auto& entry : dir.get_entries())
	{
		fmt.get_stream() << "\n\n";
		fmt.print_structure("POGO entry", entry.get_descriptor(), std::array{
			value_info{"start_rva"},
			value_info{"size"}
		});
		fmt.print_field_name("Name");
		fmt.print_packed_string(entry.get_name());
	}
}

void dump_debug_directory(formatter& fmt,
	const pe_bliss::debug::iltcg_debug_directory&)
{
	fmt.print_string("Incremental link-time code generation enabled");
}

void dump_mpx_flags(formatter& fmt,
	pe_bliss::debug::mpx_flags::value flags,
	std::size_t left_padding)
{
	using enum pe_bliss::debug::mpx_flags::value;
	fmt.print_flags(flags, left_padding, {
		{ enable, "IMAGE_MPX_ENABLE" },
		{ enable_driver_runtime,
			"IMAGE_MPX_ENABLE_DRIVER_RUNTIME" },
		{ enable_fast_fail_on_bnd_exception,
			"IMAGE_MPX_ENABLE_FAST_FAIL_ON_BND_EXCEPTION" }
	});
}

void dump_debug_directory(formatter& fmt,
	const pe_bliss::debug::mpx_debug_directory_details& dir)
{
	fmt.print_structure("MPX descriptor", dir.get_descriptor(), std::array{
		value_info{"signature"},
		value_info{"unknown1"},
		value_info{"flags", true, std::bind(
			dump_mpx_flags, std::ref(fmt),
			dir.get_flags(), std::placeholders::_1)},
		value_info{"unknown2"},
		value_info{"unknown3"},
	});
}

void dump_debug_directory(formatter& fmt,
	const pe_bliss::debug::repro_debug_directory_details& dir)
{
	if (!dir.get_descriptor())
		return;

	fmt.print_structure("Repro descriptor", *dir.get_descriptor(), std::array{
		value_info{"length"},
	});

	if (dir.get_hash())
		fmt.print_bytes("Hash", *dir.get_hash());
}

void dump_debug_directory(formatter& fmt,
	const pe_bliss::debug::spgo_debug_directory_details& dir)
{
	fmt.print_structure_name("SPGO string");
	fmt.get_stream() << ": ";
	fmt.print_packed_string(dir.get_string());
}

void dump_ex_dll_characteristics(formatter& fmt,
	pe_bliss::debug::image_dllcharacteristics_ex::value flags,
	std::size_t left_padding)
{
	using enum pe_bliss::debug::image_dllcharacteristics_ex::value;
	fmt.print_flags(flags, left_padding, {
		{ cet_compat, "CET_COMPAT" },
		{ cet_compat_strict_mode, "CET_COMPAT_STRICT_MODE" },
		{ cet_set_context_ip_validation_relaxed_mode, "CET_SET_CONTEXT_IP_VALIDATION_RELAXED_MODE" },
		{ cet_dynamic_apis_allow_in_proc, "CET_DYNAMIC_APIS_ALLOW_IN_PROC" },
		{ cet_reserved_1, "CET_RESERVED_1" },
		{ cet_reserved_2, "CET_RESERVED_2" }
	});
}

void dump_debug_directory(formatter& fmt,
	const pe_bliss::debug::ex_dllcharacteristics_debug_directory_details& dir)
{
	fmt.print_structure("Ex DLL Characteristics descriptor", dir.get_descriptor(), std::array{
		value_info{"flags", true, std::bind(
			dump_ex_dll_characteristics, std::ref(fmt),
			dir.get_characteristics(), std::placeholders::_1)},
	});
}

void dump_debug_directory(formatter& fmt,
	const pe_bliss::debug::pdb_hash_debug_directory_details& dir)
{
	fmt.print_structure_name("PDB hash debug directory");
	fmt.get_stream() << '\n';
	fmt.print_string("Algorithm");
	fmt.print_packed_string(dir.get_algorithm());
	fmt.get_stream() << '\n';
	fmt.print_bytes("Hash", dir.get_hash());
}

void dump_debug_directory(formatter& fmt,
	const pe_bliss::debug::mpdb_debug_directory_details& dir)
{
	fmt.print_structure("MPDB debug directory", dir.get_descriptor(), std::array{
		value_info{"signature"},
		value_info{"uncompressed_size"}
	});
	fmt.print_structure_name("Compressed size");
	fmt.get_stream() << ": ";
	fmt.print_value(dir.get_compressed_pdb().size());
}
} //namespace

void dump_debug(formatter& fmt, const pe_bliss::image::image& image) try
{
	auto debug_entries = pe_bliss::debug::load(image, {});
	if (!debug_entries)
		return;

	fmt.get_stream() << "===== ";
	fmt.print_structure_name("Debug directory");
	fmt.get_stream() << " =====\n\n";
	fmt.print_errors(*debug_entries);

	for (const auto& dir : debug_entries->get_entries())
	{
		fmt.print_structure("Debug directory descriptor", dir.get_descriptor(), std::array{
			value_info{"characteristics"},
			value_info{"time_date_stamp"},
			value_info{"major_version"},
			value_info{"minor_version"},
			value_info{"type", true, std::bind(
				print_debug_type, std::ref(fmt), dir.get_type())},
			value_info{"size_of_data"},
			value_info{"address_of_raw_data"},
			value_info{"pointer_to_raw_data"}
		});

		fmt.print_errors(dir);

		try
		{
			std::visit([&fmt](const auto& underlying_dir) {
				dump_debug_directory(fmt, underlying_dir);
				if constexpr (std::is_base_of_v<pe_bliss::error_list,
					std::remove_cvref_t<decltype(underlying_dir)>>)
				{
					fmt.get_stream() << "\n\n";
					fmt.print_errors(underlying_dir);
				}
			}, dir.get_underlying_directory());
		}
		catch (const std::system_error&)
		{
			//Unsupported directory type
		}

		fmt.get_stream() << "\n\n";
	}
}
catch (const std::system_error& e)
{
	fmt.print_error("Error loading debug:", e);
}
catch (const std::exception& e)
{
	fmt.print_error("Error loading debug:", e);
}
