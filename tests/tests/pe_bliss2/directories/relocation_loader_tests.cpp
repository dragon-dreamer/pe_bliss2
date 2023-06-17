#include "gtest/gtest.h"

#include <array>
#include <cstddef>
#include <cstdint>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/relocations/relocation_directory_loader.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/pe_types.h"

#include "tests/pe_bliss2/image_helper.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;

namespace
{
class RelocationLoaderTestFixture : public ::testing::Test
{
public:
	RelocationLoaderTestFixture()
		: instance(create_test_image({ .start_section_rva = section_rva,
			.sections = { { section_virtual_size, section_raw_size } } }))
	{
		instance.get_section_data_list()[0].data()
			->set_absolute_offset(absolute_offset);
	}

	void add_relocation_dir(std::uint32_t size)
	{
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::basereloc).get()
			= { .virtual_address = section_rva, .size = size };
	}

	void add_relocation_dir_to_headers()
	{
		instance.get_full_headers_buffer().copied_data().resize(0x1000);
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::basereloc).get()
			= { .virtual_address = 0x20u, .size = sizeof(std::uint32_t) * 2u };
	}

	void add_virtual_relocation_dir()
	{
		instance.get_data_directories().get_directory(
			core::data_directories::directory_type::basereloc).get()
			= { .virtual_address = section_rva + section_raw_size - 1u,
			.size = sizeof(std::uint32_t) * 2u };
	}

	void add_relocations()
	{
		auto& data = instance.get_section_data_list()[0].copied_data();
		std::copy(relocs.begin(), relocs.end(), data.begin());
	}

	static void validate_block1(relocations::relocation_directory& dir)
	{
		const auto& block1 = dir.relocations[0];
		expect_contains_errors(block1);
		EXPECT_EQ(block1.get_descriptor().get_state().absolute_offset(), absolute_offset);
		EXPECT_EQ(block1.get_descriptor().get_state().relative_offset(), 0u);
		EXPECT_EQ(block1.get_descriptor()->virtual_address, block1_rva);
		EXPECT_EQ(block1.get_descriptor()->size_of_block, block1_size);
		ASSERT_EQ(block1.get_relocations().size(), 1u);
		EXPECT_EQ(block1.get_relocations()[0].get_type(), block1_elem1_type);
	}

public:
	image::image instance;

public:
	static constexpr std::uint32_t absolute_offset = 0x1000u;
	static constexpr std::uint32_t section_rva = 0x1000u;
	static constexpr std::uint32_t section_virtual_size = 0x2000u;
	static constexpr std::uint32_t section_raw_size = 0x1000u;

	static constexpr std::uint32_t block1_rva = 0x04030201;
	static constexpr std::uint32_t block1_size
		= sizeof(std::uint16_t) * 1u + sizeof(std::uint32_t) * 2u;

	static constexpr auto block1_elem1_type = relocations::relocation_type::absolute;

	static constexpr std::array block2_elem_types{
		relocations::relocation_type::highlow,
		relocations::relocation_type::dir64,
		relocations::relocation_type::high,
		relocations::relocation_type::low,
		relocations::relocation_type::mips_jmpaddr,
		relocations::relocation_type::highadj,
		relocations::relocation_type::highadj
	};
	static constexpr std::array block2_elem_addresses{
		0xabcu, 0xdefu, 0x012u, 0xef1u, 0x89bu, 0x345u, 0x678u
	};
	static constexpr std::uint32_t block2_highadj1_param = 0x0123u;
	static constexpr std::uint32_t block2_rva = 0x08070605;
	static constexpr std::uint32_t block2_size
		= static_cast<std::uint32_t>(
			sizeof(std::uint16_t) * (1u + block2_elem_types.size())
			+ sizeof(std::uint32_t) * 2u);

	static constexpr std::array relocs{
		//Block 1
		std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, //virtual_address
		std::byte{block1_size}, std::byte{}, std::byte{}, std::byte{}, //size_of_block
		std::byte{}, std::byte{}, //absolute
		//Block 2 (unaligned)
		std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8}, //virtual_address
		std::byte{block2_size}, std::byte{}, std::byte{}, std::byte{}, //size_of_block
		std::byte{0xbc}, std::byte{0x3a}, //highlow
		std::byte{0xef}, std::byte{0xad}, //dir64
		std::byte{0x12}, std::byte{0x10}, //high
		std::byte{0xf1}, std::byte{0x2e}, //low
		std::byte{0x9b}, std::byte{0x58}, //mips_jmpaddr
		std::byte{0x45}, std::byte{0x43}, //highadj
		std::byte{0x23}, std::byte{0x01}, //block2_highadj1_param
		std::byte{0x78}, std::byte{0x46}, //highadj, no param
	};
};
} //namespace

TEST_F(RelocationLoaderTestFixture, AbsentDirectory)
{
	EXPECT_FALSE(relocations::load(instance));
}

TEST_F(RelocationLoaderTestFixture, ZeroDirectory)
{
	add_relocation_dir(block1_size + block2_size);
	auto dir = relocations::load(instance);
	ASSERT_TRUE(dir);
	expect_contains_errors(dir->errors,
		relocations::relocation_directory_loader_errc::invalid_directory_size);
	//Each entry is zero-filled, and each entry takes 4 * 2 bytes
	EXPECT_EQ(dir->relocations.size(),
		(relocs.size() + sizeof(std::uint32_t) * 2 - 1) / (sizeof(std::uint32_t) * 2));
	for (const auto& block : dir->relocations)
	{
		expect_contains_errors(block,
			relocations::relocation_directory_loader_errc::invalid_relocation_block_size);
	}
}

TEST_F(RelocationLoaderTestFixture, RelocDirectoryCutHighadj)
{
	add_relocation_dir(block1_size + block2_size);
	add_relocations();
	auto dir = relocations::load(instance);
	ASSERT_TRUE(dir);
	expect_contains_errors(dir->errors);
	ASSERT_EQ(dir->relocations.size(), 2u);

	validate_block1(*dir);

	const auto& block2 = dir->relocations[1];
	expect_contains_errors(block2,
		relocations::relocation_directory_loader_errc::unaligned_relocation_entry);
	EXPECT_EQ(block2.get_descriptor().get_state().absolute_offset(),
		absolute_offset + block1_size);
	EXPECT_EQ(block2.get_descriptor().get_state().relative_offset(), block1_size);
	EXPECT_EQ(block2.get_descriptor()->virtual_address, block2_rva);
	EXPECT_EQ(block2.get_descriptor()->size_of_block, block2_size);
	ASSERT_EQ(block2.get_relocations().size(), block2_elem_types.size());
	for (std::size_t i = 0; i != block2_elem_types.size(); ++i)
	{
		EXPECT_EQ(block2.get_relocations()[i].get_type(), block2_elem_types[i]);
		EXPECT_EQ(block2.get_relocations()[i].get_address(), block2_elem_addresses[i]);
		if (block2_elem_types[i] != relocations::relocation_type::mips_jmpaddr
			&& i != block2_elem_types.size() - 1u)
		{
			expect_contains_errors(block2.get_relocations()[i]);
		}
	}

	//mips_jmpaddr
	expect_contains_errors(block2.get_relocations()[4],
		relocations::relocation_entry_errc::unsupported_relocation_type);
	//highadj without parameter
	expect_contains_errors(block2.get_relocations()[6],
		relocations::relocation_entry_errc::relocation_param_is_absent);
	EXPECT_FALSE(block2.get_relocations()[6].get_param());

	//highadj with parameter
	ASSERT_TRUE(block2.get_relocations()[5].get_param());
	EXPECT_EQ(block2.get_relocations()[5].get_param()->get(), block2_highadj1_param);
}

TEST_F(RelocationLoaderTestFixture, RelocDirectoryValid)
{
	add_relocation_dir(block1_size);
	add_relocations();
	auto dir = relocations::load(instance);
	ASSERT_TRUE(dir);
	expect_contains_errors(dir->errors);
	ASSERT_EQ(dir->relocations.size(), 1u);
	validate_block1(*dir);
}

TEST_F(RelocationLoaderTestFixture, HeaderDirectoryError)
{
	add_relocation_dir_to_headers();
	auto dir = relocations::load(instance, { .include_headers = false });
	ASSERT_TRUE(dir);
	expect_contains_errors(dir->errors);
	ASSERT_EQ(dir->relocations.size(), 1u);
	expect_contains_errors(dir->relocations[0],
		relocations::relocation_directory_loader_errc::invalid_relocation_entry);
}

TEST_F(RelocationLoaderTestFixture, HeaderDirectory)
{
	add_relocation_dir_to_headers();
	auto dir = relocations::load(instance, { .include_headers = true });
	ASSERT_TRUE(dir);
	expect_contains_errors(dir->errors);
	ASSERT_EQ(dir->relocations.size(), 1u);
	expect_contains_errors(dir->relocations[0],
		relocations::relocation_directory_loader_errc::invalid_relocation_block_size);
}

TEST_F(RelocationLoaderTestFixture, VirtualDirectoryError)
{
	add_virtual_relocation_dir();
	auto dir = relocations::load(instance, { .allow_virtual_data = false });
	ASSERT_TRUE(dir);
	expect_contains_errors(dir->errors);
	ASSERT_EQ(dir->relocations.size(), 1u);
	expect_contains_errors(dir->relocations[0],
		relocations::relocation_directory_loader_errc::invalid_relocation_entry);
}

TEST_F(RelocationLoaderTestFixture, VirtualDirectory)
{
	add_virtual_relocation_dir();
	auto dir = relocations::load(instance, { .allow_virtual_data = true });
	ASSERT_TRUE(dir);
	expect_contains_errors(dir->errors);
	ASSERT_EQ(dir->relocations.size(), 1u);
	expect_contains_errors(dir->relocations[0],
		relocations::relocation_directory_loader_errc::invalid_relocation_block_size,
		relocations::relocation_directory_loader_errc::unaligned_relocation_entry);
}
