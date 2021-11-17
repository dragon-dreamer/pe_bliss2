#include "pe_bliss2/exceptions/arm64/arm64_exception_directory_loader.h"

#include <cassert>
#include <cstdint>
#include <variant>

#include "pe_bliss2/data_directories.h"
#include "pe_bliss2/exceptions/arm64/arm64_exception_directory.h"
#include "pe_bliss2/exceptions/arm_common/arm_common_exception_directory_loader.h"
#include "pe_bliss2/load_config/load_config_directory_loader.h"
#include "pe_bliss2/file_header.h"
#include "pe_bliss2/image.h"

namespace
{

using namespace pe_bliss;
using namespace pe_bliss::exceptions::arm64;

struct exception_directory_control
{
	static pe_bliss::exceptions::arm_common::exception_directory_info get_exception_directory(
		const image& instance, const loader_options& options)
	{
		if (!instance.is_64bit())
			return {};

		if (instance.get_file_header().get_machine_type() == file_header::machine_type::arm64)
		{
			if (instance.get_data_directories().has_exception_directory())
			{
				auto data_dir = instance.get_data_directories().get_directory(
					data_directories::directory_type::exception);

				return { data_dir->virtual_address, data_dir->size };
			}
		}

		if (instance.get_file_header().get_machine_type() == file_header::machine_type::amd64
			&& options.load_hybrid_pe_directory)
		{
			try
			{
				auto load_config_dir = load_config::load(instance, {
					.include_headers = options.include_headers,
					.allow_virtual_data = options.allow_virtual_data,
					.load_lock_prefix_table = false,
					.load_safeseh_handler_table = false,
					.load_cf_guard_function_table = false,
					.load_cf_guard_longjump_table = false,
					.load_cf_guard_export_suppression_table = false,
					.load_chpe_metadata = true,
					.load_dynamic_relocation_table = false,
					.load_enclave_config = false,
					.load_volatile_metadata = false,
					.load_ehcont_targets = false,
					.load_xfg_type_based_hashes = false
				});

				if (load_config_dir)
				{
					if (const auto* load_config_dir64 = std::get_if<
						load_config::load_config_directory_details::underlying_type64>(&load_config_dir->get_value());
						load_config_dir64)
					{
						if (const auto* metadata = std::get_if<load_config::chpe_arm64x_metadata_details>(
							&load_config_dir64->get_chpe_metadata()); metadata)
						{
							const auto& descriptor = metadata->get_metadata();
							return { descriptor->extra_rfe_table, descriptor->extra_rfe_table_size };
						}
					}
				}
			}
			catch (const std::system_error&)
			{
			}
		}

		return {};
	}
};

struct uwop_control
{
	static unwind_code decode_unwind_code(std::byte value)
	{
		return ::pe_bliss::exceptions::arm64::decode_unwind_code(value);
	}

	static void create_uwop_code(extended_unwind_record::unwind_code_list_type& unwind_codes, unwind_code code)
	{
		using enum unwind_code;
		switch (code)
		{
		case alloc_s:
			unwind_codes.emplace_back(std::in_place_type<opcode::alloc_s>);
			break;
		case save_r19r20_x:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_r19r20_x>);
			break;
		case save_fplr:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_fplr>);
			break;
		case save_fplr_x:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_fplr_x>);
			break;
		case alloc_m:
			unwind_codes.emplace_back(std::in_place_type<opcode::alloc_m>);
			break;
		case save_regp:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_regp>);
			break;
		case save_regp_x:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_regp_x>);
			break;
		case save_reg:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_reg>);
			break;
		case save_reg_x:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_reg_x>);
			break;
		case save_lrpair:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_lrpair>);
			break;
		case save_fregp:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_fregp>);
			break;
		case save_fregp_x:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_fregp_x>);
			break;
		case save_freg:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_freg>);
			break;
		case save_freg_x:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_freg_x>);
			break;
		case alloc_l:
			unwind_codes.emplace_back(std::in_place_type<opcode::alloc_l>);
			break;
		case set_fp:
			unwind_codes.emplace_back(std::in_place_type<opcode::set_fp>);
			break;
		case add_fp:
			unwind_codes.emplace_back(std::in_place_type<opcode::add_fp>);
			break;
		case nop:
			unwind_codes.emplace_back(std::in_place_type<opcode::nop>);
			break;
		case end:
			unwind_codes.emplace_back(std::in_place_type<opcode::end>);
			break;
		case end_c:
			unwind_codes.emplace_back(std::in_place_type<opcode::end_c>);
			break;
		case save_next:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_next>);
			break;
		case save_reg_any:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_reg_any>);
			break;
		case reserved_custom_stack:
			unwind_codes.emplace_back(std::in_place_type<opcode::reserved_custom_stack>);
			break;
		case reserved2:
			unwind_codes.emplace_back(std::in_place_type<opcode::reserved2>);
			break;
		default:
			assert(false);
			break;
		}
	}
};

} //namespace

namespace pe_bliss::exceptions::arm64
{

void load(const image& instance, const loader_options& options,
	pe_bliss::exceptions::exception_directory_details& directory)
{
	arm_common::load<exception_directory_control, uwop_control,
		packed_unwind_data, extended_unwind_record, exception_directory_details>(instance, options, directory);
}

} //namespace pe_bliss::exceptions::arm64
