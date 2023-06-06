#include "pe_bliss2/dotnet/dotnet_directory_loader.h"

#include <array>
#include <cstdint>
#include <vector>

#include "gtest/gtest.h"

#include "pe_bliss2/image/image.h"
#include "tests/pe_bliss2/image_helper.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;

namespace
{
class DotnetLoaderTestFixture : public ::testing::Test
{
public:
	DotnetLoaderTestFixture()
		: instance(create_test_image({ .start_section_rva = section_rva,
			.sections = { { section_virtual_size, section_raw_size } } }))
	{
		instance.get_section_data_list()[0].data()
			->set_absolute_offset(absolute_offset);
	}

	void add_dotnet_dir(std::uint32_t size = dotnet_directory_size)
	{
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::com_descriptor).get()
			= { .virtual_address = section_rva, .size = size };
	}

	void add_dotnet_dir_to_headers()
	{
		instance.get_full_headers_buffer().copied_data().resize(0x100);
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::com_descriptor).get()
			= { .virtual_address = 0x20u, .size = dotnet_directory_size };
	}

	void add_virtual_dotnet_dir()
	{
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::com_descriptor).get()
			= { .virtual_address = section_rva + section_raw_size - 1u,
			.size = dotnet_directory_size };
	}

	void add_dotnet_data()
	{
		auto& data = instance.get_section_data_list()[0].copied_data();
		std::copy(dotnet_directory.begin(),
			dotnet_directory.end(),
			data.begin());
		std::copy(metadata.begin(),
			metadata.end(),
			data.begin() + (metadata_rva - section_rva));
		std::copy(resources.begin(),
			resources.end(),
			data.begin() + (resources_rva - section_rva));
	}

public:
	image::image instance;

public:
	static constexpr std::uint32_t absolute_offset = 0x1000u;
	static constexpr std::uint32_t section_rva = 0x1000u;
	static constexpr std::uint32_t section_virtual_size = 0x2000u;
	static constexpr std::uint32_t section_raw_size = 0x1000u;
	static constexpr std::uint32_t dotnet_directory_size = 0x48u;

	static constexpr std::uint32_t metadata_rva = section_rva + 0x100u;

	static constexpr std::array metadata{
		std::byte{1}, std::byte{1}, std::byte{2}, std::byte{2},
		std::byte{5}, std::byte{5}, std::byte{7}, std::byte{7},
	};

	static constexpr std::uint32_t resources_rva = section_rva + 0x1000u - 3u;
	static constexpr std::array resources{
		std::byte{'a'}, std::byte{'b'}, std::byte{'c'}
	};

	static constexpr std::array dotnet_directory{
		std::byte{dotnet_directory_size}, std::byte{}, std::byte{}, std::byte{}, //cb
		std::byte{1}, std::byte{}, //major_runtime_version
		std::byte{1}, std::byte{}, //minor_runtime_version
		std::byte{metadata_rva & 0xffu},
		std::byte{(metadata_rva >> 8u) & 0xffu},
		std::byte{(metadata_rva >> 16u) & 0xffu},
		std::byte{(metadata_rva >> 24u) & 0xffu}, //metadata RVA
		std::byte{metadata.size()}, std::byte{}, std::byte{}, std::byte{}, //metadata size
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //flags
		std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8}, //entry_point_token_or_rva
		std::byte{resources_rva & 0xffu},
		std::byte{(resources_rva >> 8u) & 0xffu},
		std::byte{(resources_rva >> 16u) & 0xffu},
		std::byte{(resources_rva >> 24u) & 0xffu}, //resources RVA
		std::byte{resources.size() + 1u}, std::byte{}, std::byte{}, std::byte{}, //resources size
		std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //strong_name_signature RVA
		std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8}, //strong_name_signature size
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //code_manager_table RVA
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //code_manager_table size
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //vtable_fixups RVA
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //vtable_fixups size
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //export_address_table_jumps RVA
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //export_address_table_jumps size
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //managed_native_header RVA
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //managed_native_header size
	};
};
} //namespace

TEST_F(DotnetLoaderTestFixture, AbsentDirectory)
{
	EXPECT_FALSE(dotnet::load(instance));
}

TEST_F(DotnetLoaderTestFixture, EmptyDirectory)
{
	add_dotnet_dir();
	auto dir = dotnet::load(instance);
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		dotnet::dotnet_directory_loader_errc::descriptor_and_directory_sizes_do_not_match,
		dotnet::dotnet_directory_loader_errc::unsupported_descriptor_size,
		dotnet::dotnet_directory_loader_errc::empty_metadata);
	EXPECT_FALSE(dir->get_resources());
	EXPECT_FALSE(dir->get_strong_name_signature());
}

TEST_F(DotnetLoaderTestFixture, NormalDirectory)
{
	add_dotnet_dir();
	add_dotnet_data();
	auto dir = dotnet::load(instance);
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		dotnet::dotnet_directory_loader_errc::unable_to_load_strong_name_signature,
		dotnet::dotnet_directory_loader_errc::virtual_resources);
	ASSERT_TRUE(dir->get_resources());
	EXPECT_FALSE(dir->get_resources()->is_copied());
	EXPECT_FALSE(dir->get_metadata().is_copied());
	EXPECT_EQ(dir->get_metadata().copied_data(), std::vector(
		metadata.begin(), metadata.end()));
	EXPECT_EQ(dir->get_resources()->copied_data(), std::vector(
		resources.begin(), resources.end()
	));
	EXPECT_FALSE(dir->get_strong_name_signature());
}

TEST_F(DotnetLoaderTestFixture, NormalDirectoryCopy1)
{
	add_dotnet_dir();
	add_dotnet_data();
	auto dir = dotnet::load(instance, { .copy_metadata_memory = true });
	ASSERT_TRUE(dir);
	ASSERT_TRUE(dir->get_resources());
	EXPECT_FALSE(dir->get_resources()->is_copied());
	EXPECT_TRUE(dir->get_metadata().is_copied());
}

TEST_F(DotnetLoaderTestFixture, NormalDirectoryCopy2)
{
	add_dotnet_dir();
	add_dotnet_data();
	auto dir = dotnet::load(instance, { .copy_resource_memory = true });
	ASSERT_TRUE(dir);
	ASSERT_TRUE(dir->get_resources());
	EXPECT_TRUE(dir->get_resources()->is_copied());
	EXPECT_FALSE(dir->get_metadata().is_copied());
}

TEST_F(DotnetLoaderTestFixture, HeaderDirectory)
{
	add_dotnet_dir_to_headers();
	auto dir = dotnet::load(instance);
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		dotnet::dotnet_directory_loader_errc::descriptor_and_directory_sizes_do_not_match,
		dotnet::dotnet_directory_loader_errc::unsupported_descriptor_size,
		dotnet::dotnet_directory_loader_errc::empty_metadata);
}

TEST_F(DotnetLoaderTestFixture, HeaderDirectoryError)
{
	add_dotnet_dir_to_headers();
	auto dir = dotnet::load(instance, { .include_headers = false });
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		dotnet::dotnet_directory_loader_errc::invalid_directory);
}

TEST_F(DotnetLoaderTestFixture, VirtualDirectory1)
{
	add_virtual_dotnet_dir();
	auto dir = dotnet::load(instance, { .allow_virtual_data = true });
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		dotnet::dotnet_directory_loader_errc::descriptor_and_directory_sizes_do_not_match,
		dotnet::dotnet_directory_loader_errc::unsupported_descriptor_size,
		dotnet::dotnet_directory_loader_errc::empty_metadata);
}

TEST_F(DotnetLoaderTestFixture, VirtualDirectory2)
{
	add_dotnet_dir();
	add_dotnet_data();
	auto dir = dotnet::load(instance, { .allow_virtual_data = true });
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		dotnet::dotnet_directory_loader_errc::unable_to_load_strong_name_signature);
	ASSERT_TRUE(dir->get_resources());
	EXPECT_EQ(dir->get_resources()->copied_data(), std::vector(
		resources.begin(), resources.end()
	));
	EXPECT_EQ(dir->get_resources()->virtual_size(), 1u);
}

TEST_F(DotnetLoaderTestFixture, VirtualDirectoryError)
{
	add_virtual_dotnet_dir();
	auto dir = dotnet::load(instance);
	ASSERT_TRUE(dir);
	expect_contains_errors(*dir,
		dotnet::dotnet_directory_loader_errc::invalid_directory);
}
