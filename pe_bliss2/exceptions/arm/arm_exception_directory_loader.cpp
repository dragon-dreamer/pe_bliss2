#include "pe_bliss2/exceptions/arm/arm_exception_directory_loader.h"

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/exceptions/arm/arm_exception_directory.h"
#include "pe_bliss2/exceptions/arm_common/arm_common_exception_directory_loader.h"
#include "pe_bliss2/load_config/load_config_directory_loader.h"
#include "pe_bliss2/image/image.h"

namespace
{

using namespace pe_bliss;
using namespace pe_bliss::exceptions::arm;

struct exception_directory_control
{
	static pe_bliss::exceptions::arm_common::exception_directory_info get_exception_directory(
		const image::image& instance, const loader_options&)
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

} //namespace

namespace pe_bliss::exceptions::arm
{

void load(const image::image& instance, const loader_options& options,
	pe_bliss::exceptions::exception_directory_details& directory)
{
	arm_common::load<exception_directory_control,
		packed_unwind_data, extended_unwind_record,
		exception_directory_details>(instance, options, directory);
}

} //namespace pe_bliss::exceptions::arm
