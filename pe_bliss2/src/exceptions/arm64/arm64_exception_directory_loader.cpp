#include "pe_bliss2/exceptions/arm64/arm64_exception_directory_loader.h"

#include <system_error>
#include <variant>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/exceptions/arm64/arm64_exception_directory.h"
#include "pe_bliss2/exceptions/arm_common/arm_common_exception_directory_loader.h"
#include "pe_bliss2/load_config/load_config_directory_loader.h"
#include "pe_bliss2/image/image.h"

namespace
{

using namespace pe_bliss;
using namespace pe_bliss::exceptions::arm64;

struct exception_directory_control final
{
	static pe_bliss::exceptions::arm_common::exception_directory_info
		get_exception_directory(const image::image& instance,
			const loader_options& options)
	{
		if (!instance.is_64bit())
			return {};

		if (instance.get_file_header().get_machine_type()
			== core::file_header::machine_type::arm64)
		{
			if (instance.get_data_directories().has_exception_directory())
			{
				auto data_dir = instance.get_data_directories().get_directory(
					core::data_directories::directory_type::exception);

				return { data_dir->virtual_address, data_dir->size };
			}
		}

		if (instance.get_file_header().get_machine_type()
			== core::file_header::machine_type::amd64
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
						load_config::load_config_directory_details::underlying_type64>(
							&load_config_dir->get_value());
						load_config_dir64)
					{
						if (const auto* metadata = std::get_if<
							load_config::chpe_arm64x_metadata_details>(
								&load_config_dir64->get_chpe_metadata()); metadata)
						{
							const auto& descriptor = metadata->get_metadata();
							return { descriptor->extra_rfe_table,
								descriptor->extra_rfe_table_size };
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

} //namespace

namespace pe_bliss::exceptions::arm64
{

void load(const image::image& instance, const loader_options& options,
	pe_bliss::exceptions::exception_directory_details& directory)
{
	arm_common::load<exception_directory_control,
		packed_unwind_data, extended_unwind_record,
		exception_directory_details>(instance, options, directory);
}

} //namespace pe_bliss::exceptions::arm64
