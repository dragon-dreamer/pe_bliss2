#include "gtest/gtest.h"

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/core/optional_header.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/exceptions/exception_directory.h"
#include "pe_bliss2/exceptions/exception_directory_loader.h"
#include "pe_bliss2/exceptions/arm64/arm64_exception_directory_loader.h"
#include "pe_bliss2/exceptions/arm_common/arm_common_exception_directory_loader.h"
#include "tests/tests/pe_bliss2/directories/arm_common_exception_helpers.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;
using namespace pe_bliss::exceptions::arm64;

TEST(Arm64ExceptionsLoaderTests, AbsentDirectory)
{
	pe_bliss::exceptions::exception_directory_details dir;
	image::image instance;
	EXPECT_NO_THROW(load(instance, {}, dir));
	EXPECT_TRUE(dir.get_directories().empty());
}

namespace
{
void ensure_directory_loaded(const pe_bliss::exceptions::exception_directory_details& dir)
{
	ASSERT_EQ(dir.get_directories().size(), 1u);
	const auto* arm_dir = std::get_if<exception_directory_details>(&dir.get_directories()[0]);
	ASSERT_NE(arm_dir, nullptr);
	expect_contains_errors(*arm_dir,
		exceptions::arm_common::exception_directory_loader_errc::invalid_directory_size);
}

image::image prepare_image()
{
	image::image instance;
	instance.get_optional_header().initialize_with<
		core::optional_header::optional_header_64_type>();
	instance.get_file_header().set_machine_type(core::file_header::machine_type::arm64);
	instance.get_data_directories().set_size(16);
	instance.get_data_directories().get_directory(
		core::data_directories::directory_type::exception).get()
		= { .virtual_address = 1u, .size = 1u };
	return instance;
}
} //namespace

TEST(Arm64ExceptionsLoaderTests, PresentDirectory)
{
	pe_bliss::exceptions::exception_directory_details dir;
	auto instance = prepare_image();
	EXPECT_NO_THROW(load(instance, {}, dir));
	ensure_directory_loaded(dir);
}

TEST(Arm64ExceptionsLoaderTests, HybridDirectory)
{
	pe_bliss::exceptions::exception_directory_details dir;
	auto instance = create_hybrid_exception_load_config_image();
	instance.get_file_header().set_machine_type(core::file_header::machine_type::amd64);
	EXPECT_NO_THROW(load(instance, {}, dir));
	ensure_directory_loaded(dir);
}

TEST(Arm64ExceptionsLoaderTests, ExceptionLoaderPresentDirectory)
{
	auto instance = prepare_image();
	auto dir = exceptions::load(instance, {});
	ensure_directory_loaded(dir);
}
