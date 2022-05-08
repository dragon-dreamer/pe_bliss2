#include "pe_bliss2/exceptions/arm/arm_exception_directory_loader.h"

#include <cassert>
#include <cstdint>
#include <variant>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/exceptions/arm/arm_exception_directory.h"
#include "pe_bliss2/exceptions/arm_common/arm_common_exception_directory_loader.h"
#include "pe_bliss2/load_config/load_config_directory_loader.h"
#include "pe_bliss2/image.h"

namespace
{

using namespace pe_bliss;
using namespace pe_bliss::exceptions::arm;

struct exception_directory_control
{
	static pe_bliss::exceptions::arm_common::exception_directory_info get_exception_directory(
		const image& instance, const loader_options&)
	{
		if (!instance.is_64bit()
			&& instance.get_file_header().get_machine_type() == core::file_header::machine_type::armnt)
		{
			if (instance.get_data_directories().has_exception_directory())
			{
				auto data_dir = instance.get_data_directories().get_directory(
					core::data_directories::directory_type::exception);

				return { data_dir->virtual_address, data_dir->size };
			}
		}

		return {};
	}
};

struct uwop_control
{
	static unwind_code decode_unwind_code(std::byte value)
	{
		return ::pe_bliss::exceptions::arm::decode_unwind_code(value);
	}

	static void create_uwop_code(extended_unwind_record::unwind_code_list_type& unwind_codes, unwind_code code)
	{
		using enum unwind_code;
		switch (code)
		{
		case alloc_s:
			unwind_codes.emplace_back(std::in_place_type<opcode::alloc_s>);
			break;

		case save_r0r12_lr:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_r0r12_lr>);
			break;

		case mov_sprx:
			unwind_codes.emplace_back(std::in_place_type<opcode::mov_sprx>);
			break;

		case save_r4rx_lr:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_r4rx_lr>);
			break;

		case save_r4rx_lr_wide:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_r4rx_lr_wide>);
			break;

		case save_d8dx:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_d8dx>);
			break;

		case alloc_s_wide:
			unwind_codes.emplace_back(std::in_place_type<opcode::alloc_s_wide>);
			break;

		case save_r0r7_lr:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_r0r7_lr>);
			break;

		case ms_specific:
			unwind_codes.emplace_back(std::in_place_type<opcode::ms_specific>);
			break;

		case ldr_lr_sp:
			unwind_codes.emplace_back(std::in_place_type<opcode::ldr_lr_sp>);
			break;

		case save_dsde:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_dsde>);
			break;

		case save_dsde_16:
			unwind_codes.emplace_back(std::in_place_type<opcode::save_dsde_16>);
			break;

		case alloc_m:
			unwind_codes.emplace_back(std::in_place_type<opcode::alloc_m>);
			break;

		case alloc_m_wide:
			unwind_codes.emplace_back(std::in_place_type<opcode::alloc_m_wide>);
			break;

		case alloc_l:
			unwind_codes.emplace_back(std::in_place_type<opcode::alloc_l>);
			break;

		case alloc_l_wide:
			unwind_codes.emplace_back(std::in_place_type<opcode::alloc_l_wide>);
			break;

		case nop:
			unwind_codes.emplace_back(std::in_place_type<opcode::nop>);
			break;

		case nop_wide:
			unwind_codes.emplace_back(std::in_place_type<opcode::nop_wide>);
			break;

		case end_nop:
			unwind_codes.emplace_back(std::in_place_type<opcode::end_nop>);
			break;

		case end_nop_wide:
			unwind_codes.emplace_back(std::in_place_type<opcode::end_nop_wide>);
			break;

		case end:
			unwind_codes.emplace_back(std::in_place_type<opcode::end>);
			break;

		case reserved:
			unwind_codes.emplace_back(std::in_place_type<opcode::reserved>);
			break;

		default:
			assert(false);
			break;
		}
	}
};

} //namespace

namespace pe_bliss::exceptions::arm
{

void load(const image& instance, const loader_options& options,
	pe_bliss::exceptions::exception_directory_details& directory)
{
	arm_common::load<exception_directory_control, uwop_control,
		packed_unwind_data, extended_unwind_record, exception_directory_details>(instance, options, directory);
}

} //namespace pe_bliss::exceptions::arm
