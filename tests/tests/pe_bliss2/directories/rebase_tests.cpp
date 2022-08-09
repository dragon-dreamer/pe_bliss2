#include "gtest/gtest.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <climits>

#include "pe_bliss2/core/optional_header.h"
#include "pe_bliss2/detail/packed_serialization.h"
#include "pe_bliss2/relocations/image_rebase.h"
#include "pe_bliss2/image/image.h"

#include "tests/tests/pe_bliss2/image_helper.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss;

namespace
{

class ImageRebaseTestFixture : public ::testing::Test
{
public:
	ImageRebaseTestFixture()
		: instance(create_test_image({ .start_section_rva = section_rva,
			.sections = { { virtual_section_size, raw_section_size } } }))
	{
		instance.get_optional_header().initialize_with<
			core::optional_header::optional_header_64_type>();
		instance.get_optional_header().set_raw_image_base(image_base);
	}

	void add_relocated_values()
	{
		auto& data = instance.get_section_data_list()[0].copied_data();
		detail::packed_serialization<>::serialize(relocated_values(), data.data());
	}

	static relocations::base_relocation_list create_relocation_directory()
	{
		relocations::base_relocation_list dir;
		dir.resize(relocation_descriptor_count);
		dir[0].get_descriptor()->virtual_address = section_rva;
		dir[1].get_descriptor()->virtual_address = section_rva + rel_high_offset;

		relocations::relocation_entry entry;
		entry.set_address(rel_absolute_offset);
		entry.set_type(relocations::relocation_type::absolute);
		dir[0].get_relocations().emplace_back(entry);

		entry.set_address(rel_highlow_offset);
		entry.set_type(relocations::relocation_type::highlow);
		dir[0].get_relocations().emplace_back(entry);

		entry.set_address(rel_high_offset - rel_high_offset);
		entry.set_type(relocations::relocation_type::high);
		dir[1].get_relocations().emplace_back(entry);

		entry.set_address(rel_low_offset - rel_high_offset);
		entry.set_type(relocations::relocation_type::low);
		dir[1].get_relocations().emplace_back(entry);

		entry.set_address(rel_highadj_offset - rel_high_offset);
		entry.set_type(relocations::relocation_type::highadj);
		entry.get_param() = rel_highadj_param;
		dir[1].get_relocations().emplace_back(entry);

		entry.set_address(rel_dir64_offset - rel_high_offset);
		entry.set_type(relocations::relocation_type::dir64);
		entry.get_param().reset();
		dir[1].get_relocations().emplace_back(entry);

		return dir;
	}

public:
	static constexpr std::uint32_t section_rva = 0x1000u;
	static constexpr std::uint32_t raw_section_size = 0x1000u;
	static constexpr std::uint32_t virtual_section_size = 0x2000u;
	static constexpr std::uint32_t rel_absolute_offset = 0u;
	static constexpr std::uint32_t rel_highlow_offset = 4u;
	static constexpr std::uint32_t rel_high_offset = 8u;
	static constexpr std::uint32_t rel_low_offset = 10u;
	static constexpr std::uint32_t rel_highadj_offset = 12u;
	static constexpr std::uint32_t rel_dir64_offset = 14u;
	static constexpr std::uint16_t rel_highadj_param = 0x9abc;

	static constexpr std::uint64_t image_base = 0xaabbccdd22334455ull;
	static constexpr std::uint64_t new_image_base = 0x0123456789012345ull;
	
	static constexpr std::uint32_t relocation_descriptor_count = 2u;

	struct relocated_values
	{
		std::uint32_t absolute = 0x12345678u;
		std::uint32_t highlow = 0xbcdef012u;
		std::uint16_t high = 0x1234u;
		std::uint16_t low = 0x5678u;
		std::uint16_t highadj = 0x9abcu;
		std::uint64_t dir64 = 0x1234567890abcdefull;
	};

public:
	image::image instance;
};

} //namespace

TEST_F(ImageRebaseTestFixture, RebaseEmpty)
{
	EXPECT_NO_THROW(relocations::rebase(instance,
		relocations::base_relocation_list{}, { .new_base = new_image_base }));
	EXPECT_EQ(instance.get_section_data_list()[0].copied_data(),
		std::vector<std::byte>(raw_section_size));
}

TEST_F(ImageRebaseTestFixture, Rebase)
{
	add_relocated_values();
	EXPECT_NO_THROW(relocations::rebase(instance,
		create_relocation_directory(), { .new_base = new_image_base }));

	relocated_values original;
	relocated_values relocated;
	const auto& data = instance.get_section_data_list()[0].copied_data();
	detail::packed_serialization<>::deserialize(relocated, data.data());

	std::uint64_t delta = new_image_base - image_base;
	EXPECT_EQ(original.absolute, relocated.absolute);
	EXPECT_EQ(original.highlow, static_cast<std::uint32_t>(
		relocated.highlow - delta));
	EXPECT_EQ(original.high, static_cast<std::uint16_t>(
		relocated.high - (delta >> 16u)));
	EXPECT_EQ(original.low, static_cast<std::uint16_t>(
		relocated.low - delta));
	auto expected_highadj_value =
		(static_cast<std::uint32_t>(original.highadj) << 16u)
			+ static_cast<std::uint32_t>(delta) + 0x8000u + rel_highadj_param;
	expected_highadj_value >>= 16u;
	EXPECT_EQ(relocated.highadj, static_cast<std::uint16_t>(expected_highadj_value));
	EXPECT_EQ(original.dir64, relocated.dir64 - delta);
}

TEST_F(ImageRebaseTestFixture, RebaseInvalid)
{
	auto dir = create_relocation_directory();
	relocations::relocation_entry entry;
	entry.set_type(relocations::relocation_type::thumb_mov32);
	dir.emplace_back().get_relocations().emplace_back(entry);
	expect_throw_pe_error([this, &dir] {
		relocations::rebase(instance, dir, {});
	}, relocations::relocation_entry_errc::unsupported_relocation_type);
	EXPECT_EQ(instance.get_section_data_list()[0].copied_data(),
		std::vector<std::byte>(raw_section_size));
}

TEST_F(ImageRebaseTestFixture, RebaseAbsentData)
{
	auto dir = create_relocation_directory();

	relocations::relocation_entry entry;
	entry.set_type(relocations::relocation_type::dir64);
	entry.set_address(0u);
	auto& reloc = dir.emplace_back();
	reloc.get_relocations().emplace_back(entry);
	reloc.get_descriptor()->virtual_address = 0x50000u;

	expect_throw_pe_error([this, &dir] {
		relocations::rebase(instance, dir, { .ignore_virtual_data  = true });
	}, relocations::rebase_errc::unable_to_rebase_inexistent_data);
}

TEST_F(ImageRebaseTestFixture, RebaseVirtualDataError)
{
	auto dir = create_relocation_directory();

	relocations::relocation_entry entry;
	entry.set_type(relocations::relocation_type::dir64);
	entry.set_address(0u);
	auto& reloc = dir.emplace_back();
	reloc.get_relocations().emplace_back(entry);
	reloc.get_descriptor()->virtual_address
		= section_rva + virtual_section_size - 3u;

	expect_throw_pe_error([this, &dir] {
		relocations::rebase(instance, dir, { .ignore_virtual_data = true });
	}, relocations::rebase_errc::unable_to_rebase_inexistent_data);
}

TEST_F(ImageRebaseTestFixture, RebaseVirtualDataIgnored)
{
	auto dir = create_relocation_directory();

	relocations::relocation_entry entry;
	entry.set_type(relocations::relocation_type::dir64);
	entry.set_address(0u);
	auto& reloc = dir.emplace_back();
	reloc.get_relocations().emplace_back(entry);
	reloc.get_descriptor()->virtual_address
		= section_rva + raw_section_size - 3u;

	expect_throw_pe_error([this, &dir] {
		relocations::rebase(instance, dir, { .ignore_virtual_data = false });
	}, relocations::rebase_errc::unable_to_rebase_inexistent_data);

	EXPECT_NO_THROW(relocations::rebase(instance, dir, {
		.new_base = new_image_base,
		.ignore_virtual_data = true
	}));

	const auto& data = instance.get_section_data_list()[0].copied_data();
	std::uint64_t delta = new_image_base - image_base;
	auto end = data.cend();
	// 3 bytes are physical, check them
	EXPECT_EQ(*--end, static_cast<std::byte>((delta & 0xff0000ull) >> (CHAR_BIT * 2u)));
	EXPECT_EQ(*--end, static_cast<std::byte>((delta & 0xff00ull) >> (CHAR_BIT * 1u)));
	EXPECT_EQ(*--end, static_cast<std::byte>((delta & 0xffull)));
}
