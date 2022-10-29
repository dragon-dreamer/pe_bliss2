#include "pe_bliss2/debug/debug_directory_loader.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "gtest/gtest.h"

#include "pe_bliss2/detail/debug/image_debug_directory.h"
#include "pe_bliss2/image/image.h"
#include "tests/tests/pe_bliss2/image_helper.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;

namespace
{
class DebugLoaderTestFixture : public ::testing::Test
{
public:
	DebugLoaderTestFixture()
		: instance(create_test_image({ .start_section_rva = section_rva,
			.sections = { { section_virtual_size, section_raw_size } } }))
	{
		instance.get_section_data_list()[0].data()
			->set_absolute_offset(absolute_offset);
		instance.get_overlay().get_buffer().data()
			->set_absolute_offset(overlay_absolute_offset);
	}

	void add_debug_dir()
	{
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::debug).get()
			= { .virtual_address = section_rva,
			.size = static_cast<std::uint32_t>(debug_directories.size()) };
	}
	
	void add_debug_dir_to_headers()
	{
		instance.get_full_headers_buffer().copied_data().resize(0x100);
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::debug).get()
			= { .virtual_address = 0x20u,
			.size = static_cast<std::uint32_t>(debug_directories.size()) };
		std::copy(debug_directories.begin(),
			debug_directories.end(),
			instance.get_full_headers_buffer().copied_data().begin() + 0x20u);
	}
	
	void add_virtual_debug_dir()
	{
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::debug).get()
			= { .virtual_address = section_rva + section_raw_size - 1u,
			.size = static_cast<std::uint32_t>(debug_directories.size()) };
	}
	
	void add_debug_data()
	{
		auto& data = instance.get_section_data_list()[0].copied_data();
		std::copy(debug_directories.begin(),
			debug_directories.end(),
			data.begin());
		std::copy(ex_dllcharacteristics_debug_data.begin(),
			ex_dllcharacteristics_debug_data.end(),
			data.begin() + debug_data_offset);
		std::copy(omap_to_src_debug_data.begin(),
			omap_to_src_debug_data.end(),
			data.begin() + debug_data_offset
			+ ex_dllcharacteristics_debug_data.size());

		auto& overlay = instance.get_overlay();
		overlay.copied_data().resize(borland_debug_data.size());
		std::copy(borland_debug_data.begin(),
			borland_debug_data.end(),
			overlay.copied_data().begin());
	}

	void validate_debug_directories(debug::debug_directory_list_details& dirs,
		const debug::loader_options& options)
	{
		auto debug_dirs = std::min(3u, options.max_debug_directories);
		ASSERT_EQ(dirs.get_entries().size(), debug_dirs);

		if (debug_dirs)
		{
			auto& dir = dirs.get_entries()[0];
			expect_contains_errors(dir);
			EXPECT_EQ(dir.get_descriptor()->type,
				detail::debug::image_debug_type::ex_dllcharacteristics);
			EXPECT_EQ(dir.get_raw_data().is_copied(), options.copy_raw_data);
			EXPECT_EQ(dir.get_raw_data().copied_data(),
				std::vector(ex_dllcharacteristics_debug_data.begin(),
					ex_dllcharacteristics_debug_data.end()));
		}

		if (debug_dirs > 1)
		{
			auto& dir = dirs.get_entries()[1];
			if (options.max_raw_data_size == 4u)
			{
				expect_contains_errors(dir,
					debug::debug_directory_loader_errc::too_big_raw_data);
				EXPECT_EQ(dir.get_raw_data().data()->size(), 0u);
			}
			else
			{
				expect_contains_errors(dir,
					debug::debug_directory_loader_errc::rva_and_file_offset_do_not_match);
				EXPECT_EQ(dir.get_descriptor()->type,
					detail::debug::image_debug_type::omap_to_src);
				EXPECT_EQ(dir.get_raw_data().is_copied(), options.copy_raw_data);
				EXPECT_EQ(dir.get_raw_data().copied_data(),
					std::vector(omap_to_src_debug_data.begin(),
						omap_to_src_debug_data.end()));
			}
		}

		if (debug_dirs > 2)
		{
			auto& dir = dirs.get_entries()[2];
			EXPECT_EQ(dir.get_descriptor()->type,
				detail::debug::image_debug_type::borland);
			if (options.include_overlay)
			{
				expect_contains_errors(dir);
				EXPECT_EQ(dir.get_raw_data().is_copied(), options.copy_raw_data);
				EXPECT_EQ(dir.get_raw_data().copied_data(),
					std::vector(borland_debug_data.begin(),
						borland_debug_data.end()));
			}
			else
			{
				expect_contains_errors(dir,
					debug::debug_directory_loader_errc::invalid_file_offset);
				EXPECT_EQ(dir.get_raw_data().data()->size(), 0u);
			}
		}
	}

public:
	image::image instance;

public:
	static constexpr std::uint32_t absolute_offset = 0x1000u;
	static constexpr std::uint32_t section_rva = 0x1000u;
	static constexpr std::uint32_t overlay_absolute_offset = 0x5000u;
	static constexpr std::uint32_t section_virtual_size = 0x2000u;
	static constexpr std::uint32_t section_raw_size = 0x1000u;
	static constexpr std::uint32_t debug_data_offset = 0x200u;

	static constexpr std::array ex_dllcharacteristics_debug_data{
		std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //ex_dllcharacteristics
	};

	static constexpr std::array omap_to_src_debug_data{
		std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8}, //rva
		std::byte{9}, std::byte{10}, std::byte{11}, std::byte{12}, //rva_to
	};

	static constexpr std::array borland_debug_data{
		std::byte{'a'}, std::byte{'b'}, std::byte{'c'},
	};

	static constexpr std::array debug_directories{
		//directory 0: ex_dllcharacteristics
		std::byte{},std::byte{}, std::byte{}, std::byte{}, //characteristics
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //time_date_stamp
		std::byte{}, std::byte{}, //major_version
		std::byte{}, std::byte{}, //minor_version
		std::byte{detail::debug::image_debug_type::ex_dllcharacteristics},
		std::byte{}, std::byte{}, std::byte{}, //type
		std::byte{ex_dllcharacteristics_debug_data.size()},
		std::byte{}, std::byte{}, std::byte{}, //size_of_data
		std::byte{}, std::byte{0x12u}, std::byte{}, std::byte{}, //address_of_raw_data
		std::byte{}, std::byte{0x12u}, std::byte{}, std::byte{}, //pointer_to_raw_data

		//directory 1: omap_to_src
		std::byte{},std::byte{}, std::byte{}, std::byte{}, //characteristics
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //time_date_stamp
		std::byte{}, std::byte{}, //major_version
		std::byte{}, std::byte{}, //minor_version
		std::byte{detail::debug::image_debug_type::omap_to_src},
		std::byte{}, std::byte{}, std::byte{}, //type
		std::byte{omap_to_src_debug_data.size()},
		std::byte{}, std::byte{}, std::byte{}, //size_of_data
		std::byte{ex_dllcharacteristics_debug_data.size()},
		std::byte{0x12u}, std::byte{}, std::byte{}, //address_of_raw_data
		std::byte{ex_dllcharacteristics_debug_data.size()},
		std::byte{0x13u}, std::byte{}, std::byte{}, //pointer_to_raw_data

		//directory 2: borland
		std::byte{},std::byte{}, std::byte{}, std::byte{}, //characteristics
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //time_date_stamp
		std::byte{}, std::byte{}, //major_version
		std::byte{}, std::byte{}, //minor_version
		std::byte{detail::debug::image_debug_type::borland},
		std::byte{}, std::byte{}, std::byte{}, //type
		std::byte{borland_debug_data.size()},
		std::byte{}, std::byte{}, std::byte{}, //size_of_data
		std::byte{}, std::byte{}, std::byte{}, std::byte{}, //address_of_raw_data
		std::byte{overlay_absolute_offset & 0xffu},
		std::byte{(overlay_absolute_offset >> 8u) & 0xffu},
		std::byte{(overlay_absolute_offset >> 16u) & 0xffu},
		std::byte{(overlay_absolute_offset >> 24u) & 0xffu}, //pointer_to_raw_data
	};
};
} //namespace

TEST_F(DebugLoaderTestFixture, AbsentDirectory)
{
	EXPECT_FALSE(debug::load(instance));
}

TEST_F(DebugLoaderTestFixture, ZeroDirectory)
{
	add_debug_dir();
	auto dirs = debug::load(instance);
	ASSERT_TRUE(dirs);
	ASSERT_EQ(dirs->get_entries().size(), 3u);
	expect_contains_errors(dirs->get_entries()[0],
		debug::debug_directory_loader_errc::no_rva_and_file_offset);
	expect_contains_errors(dirs->get_entries()[1],
		debug::debug_directory_loader_errc::no_rva_and_file_offset);
	expect_contains_errors(dirs->get_entries()[2],
		debug::debug_directory_loader_errc::no_rva_and_file_offset);
}

TEST_F(DebugLoaderTestFixture, NormalDirectory)
{
	add_debug_dir();
	add_debug_data();
	auto dirs = debug::load(instance);
	ASSERT_TRUE(dirs);
	expect_contains_errors(*dirs);
	validate_debug_directories(*dirs, {});
}

TEST_F(DebugLoaderTestFixture, NormalDirectoryCopyData)
{
	add_debug_dir();
	add_debug_data();
	auto dirs = debug::load(instance, { .copy_raw_data = true });
	ASSERT_TRUE(dirs);
	expect_contains_errors(*dirs);
	validate_debug_directories(*dirs, { .copy_raw_data = true });
}

TEST_F(DebugLoaderTestFixture, NormalDirectoryLimitDirs)
{
	add_debug_dir();
	add_debug_data();
	auto dirs = debug::load(instance, { .max_debug_directories = 2u });
	ASSERT_TRUE(dirs);
	expect_contains_errors(*dirs,
		debug::debug_directory_loader_errc::too_many_debug_directories);
	validate_debug_directories(*dirs, { .max_debug_directories = 2u });
}

TEST_F(DebugLoaderTestFixture, NormalDirectoryLimitSize)
{
	add_debug_dir();
	add_debug_data();
	auto dirs = debug::load(instance, { .max_raw_data_size = 4u });
	ASSERT_TRUE(dirs);
	expect_contains_errors(*dirs);
	validate_debug_directories(*dirs, { .max_raw_data_size = 4u });
}

TEST_F(DebugLoaderTestFixture, NormalDirectoryNoOverlay)
{
	add_debug_dir();
	add_debug_data();
	auto dirs = debug::load(instance, { .include_overlay = false });
	ASSERT_TRUE(dirs);
	expect_contains_errors(*dirs);
	validate_debug_directories(*dirs, { .include_overlay = false });
}

TEST_F(DebugLoaderTestFixture, HeadersDirectory)
{
	add_debug_dir_to_headers();
	add_debug_data();
	auto dirs = debug::load(instance);
	ASSERT_TRUE(dirs);
	expect_contains_errors(*dirs);
	validate_debug_directories(*dirs, {});
}

TEST_F(DebugLoaderTestFixture, HeadersDirectoryError)
{
	add_debug_dir_to_headers();
	add_debug_data();
	auto dirs = debug::load(instance, { .include_headers = false });
	ASSERT_TRUE(dirs);
	expect_contains_errors(*dirs,
		debug::debug_directory_loader_errc::unable_to_load_entries);
	EXPECT_TRUE(dirs->get_entries().empty());
}

TEST_F(DebugLoaderTestFixture, VirtualDirectoryError)
{
	add_virtual_debug_dir();
	auto dirs = debug::load(instance);
	ASSERT_TRUE(dirs);
	expect_contains_errors(*dirs,
		debug::debug_directory_loader_errc::unable_to_load_entries);
	EXPECT_TRUE(dirs->get_entries().empty());
}

TEST_F(DebugLoaderTestFixture, VirtualDirectory)
{
	add_virtual_debug_dir();
	auto dirs = debug::load(instance, { .allow_virtual_data = true });
	ASSERT_TRUE(dirs);
	expect_contains_errors(*dirs);
	EXPECT_EQ(dirs->get_entries().size(), 3u);
}

TEST_F(DebugLoaderTestFixture, NormalDirectoryExcessiveData)
{
	add_debug_dir();
	add_debug_data();
	instance.get_data_directories().get_directory(
		core::data_directories::directory_type::debug)->size++;
	auto dirs = debug::load(instance);
	expect_contains_errors(*dirs,
		debug::debug_directory_loader_errc::excessive_data_in_directory);
	validate_debug_directories(*dirs, {});
}
