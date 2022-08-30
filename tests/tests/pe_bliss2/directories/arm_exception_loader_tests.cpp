#include "gtest/gtest.h"

#include <variant>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/exceptions/exception_directory.h"
#include "pe_bliss2/exceptions/arm/arm_exception_directory_loader.h"
#include "pe_bliss2/exceptions/arm_common/arm_common_exception_directory_loader.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::exceptions::arm;

TEST(ArmExceptionsLoaderTests, AbsentDirectory)
{
	pe_bliss::exceptions::exception_directory_details dir;
	image::image instance;
	EXPECT_NO_THROW(load(instance, {}, dir));
	EXPECT_TRUE(dir.get_directories().empty());
}

TEST(ArmExceptionsLoaderTests, PresentDirectory)
{
	pe_bliss::exceptions::exception_directory_details dir;
	image::image instance;
	instance.get_file_header().set_machine_type(core::file_header::machine_type::armnt);
	instance.get_data_directories().set_size(16);
	instance.get_data_directories().get_directory(
		core::data_directories::directory_type::exception).get()
		= { .virtual_address = 100u, .size = 1u };
	EXPECT_NO_THROW(load(instance, {}, dir));
	ASSERT_EQ(dir.get_directories().size(), 1u);
	const auto* arm_dir = std::get_if<exception_directory_details>(&dir.get_directories()[0]);
	ASSERT_NE(arm_dir, nullptr);
	expect_contains_errors(*arm_dir,
		exceptions::arm_common::exception_directory_loader_errc::unmatched_directory_size);
}
